#include "engine.h"

#include <libtorrent/session.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/error_code.hpp>
#include <libtorrent/write_resume_data.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

namespace {
std::atomic_bool stopping{false};

void on_signal(int) { stopping.store(true, std::memory_order_relaxed); }

bool is_magnet(const char* s) {
    return std::strncmp(s, "magnet:?", 8) == 0;
}

void print_alerts(lt::session& ses) {
    std::vector<lt::alert*> alerts;
    ses.pop_alerts(&alerts);
    for (lt::alert const* a : alerts) {
        if (auto* e = lt::alert_cast<lt::torrent_error_alert>(a)) {
            std::cerr << "[TORRENT ERROR] " << e->message() << '\n';
        } else if (auto* e = lt::alert_cast<lt::file_error_alert>(a)) {
            std::cerr << "[FILE ERROR] " << e->message() << '\n';
        } else if (auto* e = lt::alert_cast<lt::metadata_failed_alert>(a)) {
            std::cerr << "[METADATA ERROR] " << e->message() << '\n';
        } else if (lt::alert_cast<lt::metadata_received_alert>(a)) {
            std::cout << "[METADATA] Received torrent metadata\n";
        } else if (auto* e = lt::alert_cast<lt::torrent_finished_alert>(a)) {
            std::cout << "[TORRENT] Finished: " << e->torrent_name() << '\n';
        } else if (auto* e = lt::alert_cast<lt::save_resume_data_failed_alert>(a)) {
            std::cerr << "[RESUME ERROR] " << e->message() << '\n';
        }
    }
}

bool save_resume_and_wait(lt::session& ses, lt::torrent_handle& h,
                          const std::filesystem::path& directory) {
    h.save_resume_data(lt::torrent_handle::only_if_modified | lt::torrent_handle::save_info_dict);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        std::vector<lt::alert*> alerts;
        ses.pop_alerts(&alerts);
        for (lt::alert const* a : alerts) {
            if (auto* rd = lt::alert_cast<lt::save_resume_data_alert>(a)) {
                const auto data = lt::write_resume_data_buf(rd->params);
                const auto file = directory / ".bittorrentclient.fastresume";
                std::ofstream out(file, std::ios::binary | std::ios::trunc);
                if (!out) return false;
                out.write(data.data(), static_cast<std::streamsize>(data.size()));
                return static_cast<bool>(out);
            }
            if (lt::alert_cast<lt::save_resume_data_failed_alert>(a)) return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return false;
}
}

extern "C" int bt_engine_run(const char* input, const char* save_path) {
    if (!input || !save_path || !*input || !*save_path) return 2;

    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);
    stopping.store(false, std::memory_order_relaxed);

    try {
        lt::settings_pack settings;
        settings.set_str(lt::settings_pack::user_agent, "bittorrentclient/1.0");
        settings.set_int(lt::settings_pack::alert_mask,
            lt::alert_category::error |
            lt::alert_category::status |
            lt::alert_category::storage |
            lt::alert_category::tracker |
            lt::alert_category::connect |
            lt::alert_category::piece_progress);
        settings.set_bool(lt::settings_pack::enable_dht, true);
        settings.set_bool(lt::settings_pack::enable_lsd, true);
        settings.set_bool(lt::settings_pack::enable_upnp, true);
        settings.set_bool(lt::settings_pack::enable_natpmp, true);

        lt::session ses(settings);
        lt::add_torrent_params atp;
        lt::error_code ec;
        std::shared_ptr<lt::torrent_info> torrent_info;

        if (is_magnet(input)) {
            atp = lt::parse_magnet_uri(input, ec);
            if (ec) {
                std::cerr << "[MAGNET ERROR] " << ec.message() << '\n';
                return 3;
            }
        } else {
            torrent_info = std::make_shared<lt::torrent_info>(input, ec);
            if (ec) {
                std::cerr << "[TORRENT ERROR] " << ec.message() << '\n';
                return 4;
            }
            atp.ti = torrent_info;
        }

        const std::filesystem::path requested(save_path);
        const bool explicit_file = !std::filesystem::exists(requested) && requested.has_extension();
        std::filesystem::path destination = explicit_file ? requested.parent_path() : requested;
        if (destination.empty()) destination = ".";
        std::filesystem::create_directories(destination, ec);
        if (ec) {
            std::cerr << "[PATH ERROR] " << ec.message() << '\n';
            return 5;
        }
        atp.save_path = destination.string();

        lt::torrent_handle h = ses.add_torrent(std::move(atp), ec);
        if (ec) {
            std::cerr << "[ADD ERROR] " << ec.message() << '\n';
            return 6;
        }

        std::cout << "[ENGINE] Production libtorrent backend started\n";
        std::cout << "[ENGINE] DHT/LPD/UPnP/NAT-PMP enabled where available\n";

        bool done = false;
        while (!stopping.load(std::memory_order_relaxed)) {
            print_alerts(ses);
            lt::torrent_status st = h.status();
            const double pct = st.progress_ppm / 10000.0;
            std::cout << "[PROGRESS] " << pct << "%"
                      << " down=" << st.download_rate / 1000 << " kB/s"
                      << " up=" << st.upload_rate / 1000 << " kB/s"
                      << " peers=" << st.num_peers << '\n';
            if (st.is_seeding) {
                done = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        if (stopping.load(std::memory_order_relaxed)) {
            if (!save_resume_and_wait(ses, h, destination))
                std::cerr << "[RESUME] Could not save fast-resume data\n";
            h.pause();
            std::cout << "[ENGINE] Stopped cleanly\n";
            return 130;
        }

        if (done && explicit_file && torrent_info && torrent_info->files().num_files() == 1) {
            const std::filesystem::path source = destination / torrent_info->files().file_path(0);
            if (source != requested) {
                std::error_code rename_ec;
                std::filesystem::remove(requested, rename_ec);
                std::filesystem::rename(source, requested, rename_ec);
                if (rename_ec) {
                    std::cerr << "[OUTPUT] Could not rename completed file: " << rename_ec.message() << '\n';
                    return 8;
                }
            }
        }

        std::cout << (done ? "[ENGINE] Download complete\n" : "[ENGINE] Download stopped\n");
        return done ? 0 : 7;
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << '\n';
        return 1;
    }
}
