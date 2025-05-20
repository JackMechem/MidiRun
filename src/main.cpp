#include "include/signal_state.hpp"
#include <atomic>
#include <cstring>
#define APP_VERSION "1.2.1"

#include <iostream>
#include <pwd.h>
#include <signal.h>
#include <string>
#include <thread>

#include "RtMidi.h"
#include <toml++/toml.hpp>

#include "../include/deamon.hpp"
#include "../include/log.hpp"
#include <thread>

#ifdef __unix__

#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>

#endif

bool done;
bool verbose = false;
std::atomic<bool> reloading = false;
static void finish(int ignore) { done = true; }

void reload() {
	LOG_INFO("Reload function called.");
	reloading = true;
}

int listMidiIO() {

	RtMidiIn *midiin = 0;
	RtMidiOut *midiout = 0;

	// RtMidiIn constructor
	try {
		midiin = new RtMidiIn();
	} catch (RtMidiError &error) {
		error.printMessage();
		exit(EXIT_FAILURE);
	}

	// Check inputs.
	unsigned int nPorts = midiin->getPortCount();
	std::cout << "There are " << nPorts << " MIDI input sources available.\n";
	std::string portName;
	for (unsigned int i = 0; i < nPorts; i++) {
		try {
			portName = midiin->getPortName(i);
		} catch (RtMidiError &error) {
			error.printMessage();
			goto cleanup;
		}
		std::cout << "  Input Port #" << i + 1 << ": " << portName << '\n';
	}

cleanup:
	delete midiin;
	delete midiout;
	return 0;
}

int listenToMidiPort(int port) {
	RtMidiIn *midiin = new RtMidiIn();
	std::vector<unsigned char> message;
	int nBytes, i;
	double stamp;

	// Check ports
	unsigned int nPorts = midiin->getPortCount();
	if (nPorts == 0) {
		std::cout << "No ports available!\n";
		goto cleanup;
	}
	if (nPorts < port) {
		std::cout << "Port does not exist!\n";
		std::cout << "See help page (midirun {--help|h}) for more info.";
	}

	midiin->openPort(port - 1);

	midiin->ignoreTypes(false, false, false);

	done = false;
	(void)signal(SIGINT, finish);

	// Periodically check input queue.
	std::cout << "Reading MIDI from port " << port << " -- quit with Ctrl-C.\n";
	while (!done) {
		stamp = midiin->getMessage(&message);
		nBytes = message.size();
		for (i = 0; i < nBytes; i++)
			std::cout << "Byte " << i << " = " << (int)message[i] << ", ";
		if (nBytes > 0)
			std::cout << "stamp = " << stamp << std::endl;

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

cleanup:
	delete midiin;
	return 0;
}

void emit(int fd, int type, int code, int val) {
	struct input_event ie;

	ie.type = type;
	ie.code = code;
	ie.value = val;
	ie.time.tv_sec = 0;
	ie.time.tv_usec = 0;

	write(fd, &ie, sizeof(ie));
}

int keyPress(int fd, std::vector<int> keyCodes) {

	for (int i = 0; i < keyCodes.size(); i++) {
		emit(fd, EV_KEY, keyCodes[i], 1);
		emit(fd, EV_SYN, SYN_REPORT, 0);
	}

	for (int i = 0; i < keyCodes.size(); i++) {
		emit(fd, EV_KEY, keyCodes[i], 0);
		emit(fd, EV_SYN, SYN_REPORT, 0);
	}

	return 0;
}

int listenAndMapDaemon(std::string configLocation, Daemon &daemon) {

	// {{{ Create Variables For Reading Midi Input
	RtMidiIn *midiin = new RtMidiIn();
	std::vector<unsigned char> message;
	int nBytes, i;
	double stamp;
	int fd;
	// }}}

	// {{{ Open Uinput File

	fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	struct uinput_setup usetup;

	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_KEYBIT, 10);
	for (ssize_t i = 1; i < KEY_MAX; i++)
		ioctl(fd, UI_SET_KEYBIT, i);

	memset(&usetup, 0, sizeof(usetup));
	usetup.id.bustype = BUS_USB;
	usetup.id.vendor = 0x1234;
	usetup.id.product = 0x5678;
	strcpy(usetup.name, "Midi to Key");

	ioctl(fd, UI_DEV_SETUP, &usetup);
	ioctl(fd, UI_DEV_CREATE);

	sleep(1);

	// }}}

	struct conf_config {
		std::optional<int> inputPort;
	};

	struct conf_mapping {
		std::string name;
		int byte0;
		int byte1;
		std::vector<int> key;
	};

	try {

		// {{{ File parsing
		toml::table parsedToml = toml::parse_file(configLocation);

		// Config Section
		conf_config configData;
		configData.inputPort = parsedToml["config"]["inputPort"].value<int>();
		if (!configData.inputPort.has_value()) {
			LOG_ERROR("NO INPUT PORT VALUE PROVIDED!");
			return 1;
		}
		LOG_INFO("PROVIDED MIDI INPUT PORT [", configData.inputPort.value(),
				 "]");

		// Mapping Section
		std::vector<conf_mapping> parsedMappingData;

		auto mappingTableArray = parsedToml["mapping"];

		toml::array mappingData = *mappingTableArray.as_array();

		for (const toml::node &mapping : mappingData) {
			toml::table mappingData = *mapping.as_table();
			conf_mapping parsedMapping;
			parsedMapping.name = mappingData["name"].value_or("NAN");
			parsedMapping.byte0 = mappingData["byte0"].value_or(128);
			parsedMapping.byte1 = mappingData["byte1"].value_or(0);
			std::vector<int> parsedKeys;
			auto keyArray = mappingData["key"];
			toml::array keyData = *keyArray.as_array();
			for (const toml::node &keyValue : keyData) {
				parsedKeys.push_back(keyValue.value_or(0));
			}
			parsedMapping.key = parsedKeys;

			parsedMappingData.push_back(parsedMapping);
		}

		// }}}

		// {{{ Midi Device And Map Loop

		// Check if there any ports just in case
		unsigned int nPorts = midiin->getPortCount();
		if (nPorts == 0) {
			LOG_ERROR("NO PORTS AVAILABLE!");
			goto cleanup;
		}
		if (nPorts < configData.inputPort.value()) {
			LOG_ERROR("PORT DOES NOT EXIST!");
			LOG_INFO("SEE HELP PAGE!");
		}

		midiin->openPort(configData.inputPort.value() - 1);

		midiin->ignoreTypes(false, false, false);

		done = false;
		(void)signal(SIGINT, finish);

		// Periodically check input queue.
		LOG_INFO("READING MIDI FROM PORT [", configData.inputPort.value(), "]");
		while (daemon.IsRunning() && reloading == false) {
			stamp = midiin->getMessage(&message);
			nBytes = message.size();
			for (i = 0; i < nBytes; i++) {
				if (verbose)
					LOG_INFO("BYTE [", i, "] [", (int)message[i], "] ");
			}
			if (nBytes > 0)
				if (verbose)
					LOG_INFO("STAMP [", stamp, "]");
			if (nBytes > 0) {
				for (int v = 0; v <= parsedMappingData.size() - 1; v++) {
					conf_mapping newMapping = parsedMappingData[v];
					if (newMapping.byte0 == (int)message[0] &&
						newMapping.byte1 == (int)message[1]) {
						if (verbose)
							LOG_INFO("TRIGGERING MAPPING [", newMapping.name,
									 "]");
						keyPress(fd, newMapping.key);
					}
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		// }}}

	cleanup:
		delete midiin;
		ioctl(fd, UI_DEV_DESTROY);
		close(fd);

		return 0;

	} catch (const toml::parse_error &err) {
		LOG_ERROR("ERROR PARSING FILE [", err.source().path,
				  "]: ", err.description(), " [", err.source().begin, "]");
		return 1;
	}

	ioctl(fd, UI_DEV_DESTROY);
	close(fd);

	return 0;
}

int listenAndMap(std::string configLocation) {

	// {{{ Create Variables For Reading Midi Input
	RtMidiIn *midiin = new RtMidiIn();
	std::vector<unsigned char> message;
	int nBytes, i;
	double stamp;
	int fd;
	// }}}

	// {{{ Open Uinput File

	fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	struct uinput_setup usetup;

	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_KEYBIT, 10);
	for (ssize_t i = 1; i < KEY_MAX; i++)
		ioctl(fd, UI_SET_KEYBIT, i);

	memset(&usetup, 0, sizeof(usetup));
	usetup.id.bustype = BUS_USB;
	usetup.id.vendor = 0x1234;
	usetup.id.product = 0x5678;
	strcpy(usetup.name, "Midi to Key");

	ioctl(fd, UI_DEV_SETUP, &usetup);
	ioctl(fd, UI_DEV_CREATE);

	sleep(1);

	// }}}

	struct conf_config {
		std::optional<int> inputPort;
	};

	struct conf_mapping {
		std::string name;
		int byte0;
		int byte1;
		std::vector<int> key;
	};

	try {

		// {{{ File parsing
		toml::table parsedToml = toml::parse_file(configLocation);

		// Config Section
		conf_config configData;
		configData.inputPort = parsedToml["config"]["inputPort"].value<int>();
		if (!configData.inputPort.has_value()) {
			std::cout << "No input value provided!\n";
			return 1;
		}
		std::cout << "Provided midi input port: "
				  << configData.inputPort.value() << std::endl
				  << std::endl;

		// Mapping Section
		std::vector<conf_mapping> parsedMappingData;

		auto mappingTableArray = parsedToml["mapping"];

		toml::array mappingData = *mappingTableArray.as_array();

		if (verbose)
			std::cout << "Config Data provided: \n";
		for (const toml::node &mapping : mappingData) {
			toml::table mappingData = *mapping.as_table();
			if (verbose)
				std::cout << mappingData << std::endl << std::endl;

			conf_mapping parsedMapping;
			parsedMapping.name = mappingData["name"].value_or("NAN");
			parsedMapping.byte0 = mappingData["byte0"].value_or(128);
			parsedMapping.byte1 = mappingData["byte1"].value_or(0);
			std::vector<int> parsedKeys;
			auto keyArray = mappingData["key"];
			toml::array keyData = *keyArray.as_array();
			for (const toml::node &keyValue : keyData) {
				parsedKeys.push_back(keyValue.value_or(0));
			}
			parsedMapping.key = parsedKeys;

			parsedMappingData.push_back(parsedMapping);
		}

		// }}}

		// {{{ Midi Device And Map Loop

		// Check if there any ports just in case
		unsigned int nPorts = midiin->getPortCount();
		if (nPorts == 0) {
			std::cout << "No ports available!\n";
			goto cleanup;
		}
		if (nPorts < configData.inputPort.value()) {
			std::cout << "Port does not exist!\n";
			std::cout << "See help page (midirun {--help|h}) for more info.";
		}

		midiin->openPort(configData.inputPort.value() - 1);

		midiin->ignoreTypes(false, false, false);

		done = false;
		(void)signal(SIGINT, finish);

		std::cout << "Reading MIDI from port " << configData.inputPort.value()
				  << " -- quit with Ctrl-C.\n";
		while (!done) {
			stamp = midiin->getMessage(&message);
			nBytes = message.size();
			for (i = 0; i < nBytes; i++) {
				if (verbose)
					std::cout << "Byte " << i << " = " << (int)message[i]
							  << ", ";
			}
			if (nBytes > 0)
				if (verbose)
					std::cout << "stamp = " << stamp << std::endl;
			if (nBytes > 0) {
				for (int v = 0; v <= parsedMappingData.size() - 1; v++) {
					conf_mapping newMapping = parsedMappingData[v];
					if (newMapping.byte0 == (int)message[0] &&
						newMapping.byte1 == (int)message[1]) {
						if (verbose)
							std::cout << "Triggering mapping named: "
									  << newMapping.name << std::endl
									  << std::endl;
						keyPress(fd, newMapping.key);
					}
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		// }}}

	cleanup:
		delete midiin;
		ioctl(fd, UI_DEV_DESTROY);
		close(fd);

		return 0;

	} catch (const toml::parse_error &err) {
		std::cerr << "Error parsing file '" << err.source().path << "':\n"
				  << err.description() << "\n (" << err.source().begin << ")\n";
		return 1;
	}

	ioctl(fd, UI_DEV_DESTROY);
	close(fd);

	return 0;
}

// }}}

std::string GetEnv(const std::string &var) {
	const char *val = std::getenv(var.c_str());
	if (val == nullptr) {
		return "";
	} else {
		return val;
	}
}

void helpMessage() {
	std::cout << "MidiRun - Version " << APP_VERSION << "\n";
	std::cout << "Made by Jack Mechem -- Project Github: "
				 "https://github.com/JackMechem/midirun \n\n";
	std::cout << "Usage:\n";
	std::cout << "  Help and List:\n";
	std::cout << "    midirun {--help|-h} | Shows Help Page\n";
	std::cout << "    midirun [{--verbose|-v}] {--list-io|-lio} | Lists midi "
				 "inputs and "
				 "outputs\n";
	std::cout << "    midirun [{--verbose|-v}] "
				 "{--listen|-ln} <Port-Number> "
				 "| Listens "
				 "to specified input port and displays midi note registered\n";
	std::cout << std::endl << std::endl;
	std::cout << "  Running The Program:\n";
	std::cout << "    midirun [{--verbose|-v}] [{--config|-c} "
				 "</path/to/config>] | Note: Default config is "
				 "$HOME/.config/midirun/config.toml\n";
}

int main(int argc, char *argv[]) {

	// {{{ Get Home Directory ()
	struct passwd *pw = getpwuid(getuid());
	const std::string homedir = pw->pw_dir;
	// }}}

	struct {
		bool run = true;
		bool verbose = false;
		bool help = false;
		bool listIO = false;
		bool listen = false;
		bool gui = false;
		bool daemonize = false;
		int listenPort;
		std::string config;
	} args;

	args.config = homedir + "/.config/midirun/config.toml";

	// {{{ Loop Through Command Args
	for (int i = 0; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "--verbose" || arg == "-v") {
			args.verbose = true;
		} else if (arg == "--help" || arg == "-h") {
			args.help = true;
			args.run = false;
		} else if (arg == "--config" || arg == "-c") {
			args.config = argv[i + 1];
		} else if (arg == "--list-io" || arg == "-lio") {
			args.listIO = true;
			args.run = false;
		} else if (arg == "--listen" || arg == "-ln") {
			args.listen = true;
			args.listenPort = atoi(argv[i + 1]);
			args.run = false;
		} else if (arg == "--gui" || arg == "-g") {
			args.run = false;
			args.gui = true;
		} else if (arg == "--daemon" || arg == "-d") {
			args.daemonize = true;
		}
	}
	if (args.verbose) {
		verbose = true;
	}

	if (verbose) {
		std::cout << "Verbose Mode Enabled\n"
				  << "--------------------\n";
	}

	if (args.help && args.listen) {
		std::cerr << "Invalid Argument(s)!\n"
				  << "See midirun --help for more info.";
		return 0;
	} else if (args.help && args.listIO) {
		std::cerr << "Invalid Argument(s)!\n"
				  << "See midirun --help for more info.\n";
		return 0;
	} else if (args.help) {
		helpMessage();
		return 0;
	}
	if (args.listIO) {
		listMidiIO();
		return 0;
	}
	if (args.listen && args.listenPort) {
		listenToMidiPort(args.listenPort);
	}

	if (args.run) {

		if (args.daemonize) {

			// The Daemon class is a singleton to avoid be instantiate more than
			// once
			Daemon &daemon = Daemon::instance();
			Daemon::setInstancePtr(&daemon);
			// Daemon::writePidFile(PID_FILE_DIR);
			LOG_INFO("CONFIG PATH [", args.config, "]");

			// Set the reload function to be called in case of receiving a
			// SIGHUP signal
			daemon.setReloadFunction(reload);
			ioctl(0, TIOCNOTTY, NULL);

			while (daemon.IsRunning()) {
				reloading = false;
				listenAndMapDaemon(args.config, daemon);
			}
			// Daemon main loop

			LOG_INFO("THE DAEMON PROCESS ENDED GRACEFULLY");
		} else {
			std::cout << "Config Path: " << args.config << std::endl;
			listenAndMap(args.config);
		}
	}
	/// }}}

	return 0;
}
