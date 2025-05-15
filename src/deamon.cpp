#include "../include/deamon.hpp"
#include "include/signal_state.hpp"
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

Daemon::Daemon() {
	signal(SIGINT, Daemon::signalHandler);
	signal(SIGTERM, Daemon::signalHandler);
	signal(SIGHUP, Daemon::signalHandler);
}

void Daemon::setReloadFunction(std::function<void()> func) {
	m_reloadFunc = func;
}

bool Daemon::IsRunning() {
	if (g_shouldReload) {
		g_shouldReload = false;
		m_reloadFunc();
	}
	if (g_shouldStop) {
		m_isRunning = false;
	} else {
		m_isRunning = true;
	}
	return m_isRunning;
}

void Daemon::signalHandler(int signal) {
	switch (signal) {
	case SIGINT:
	case SIGTERM: {
		g_shouldStop = true;
		break;
	}
	case SIGHUP: {
		g_shouldReload = true;
		break;
	}
	}
}

Daemon *Daemon::s_instance = nullptr;

void Daemon::setInstancePtr(Daemon *ptr) { s_instance = ptr; }
