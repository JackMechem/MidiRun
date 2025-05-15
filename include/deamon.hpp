#pragma once

#include <csignal>
#include <functional>
#include <unistd.h>

#define PID_FILE_DIR "/run/midirundae"

class Daemon {
  public:
	static Daemon &instance() {
		static Daemon instance;
		return instance;
	}
	static void setInstancePtr(Daemon *ptr);
	// static void writePidFile(const std::string &path) {
	// 	std::ofstream pidfile(path);
	// 	if (pidfile.is_open()) {
	// 		pidfile << getpid();
	// 		pidfile.close();
	// 		LOG_INFO("Wrote PID File [", PID_FILE_DIR, "]");
	// 	}
	// }

	// static bool stop(const std::string &pidfilePath) {

	// 	std::ifstream pidfile(pidfilePath);
	// 	pid_t pid;
	// 	if (pidfile >> pid) {
	// 		if (kill(pid, SIGTERM) == 0) {
	// 			std::cout << "Stopped Midirun Daemon (PID " << pid << ")."
	// 					  << std::endl;
	// 			std::remove(PID_FILE_DIR);
	// 			return true;
	// 		} else {
	// 			perror("kill");
	// 		}
	// 	} else {
	// 		std::cerr << "Failed to read PID file or daemon not running."
	// 				  << std::endl;
	// 	}
	// 	return false;
	// }

	void setReloadFunction(std::function<void()> func);

	bool IsRunning();

  private:
	std::function<void()> m_reloadFunc;
	bool m_isRunning;
	bool m_reload;

	Daemon();
	Daemon(Daemon const &) = delete;
	void operator=(Daemon const &) = delete;
	static Daemon *s_instance;

	void Reload();

	static void signalHandler(int signal);
};
