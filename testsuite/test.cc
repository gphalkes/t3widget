#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <sys/time.h>
#include <sys/resource.h>
#include <csignal>

#include "widget.h"

using namespace t3_widget;

static bool option_test_mode;
static FILE *log_file;

static const char *executable;
static void start_debugger_on_segfault(int sig) {
	static char debug_buffer[1024];
	struct rlimit vm_limit;

	fprintf(stderr, "Handling signal %d\n", sig);

	signal(SIGSEGV, SIG_DFL);
	signal(SIGABRT, SIG_DFL);

	getrlimit(RLIMIT_AS, &vm_limit);
	vm_limit.rlim_cur = vm_limit.rlim_max;
	setrlimit(RLIMIT_AS, &vm_limit);
	sprintf(debug_buffer, "DISPLAY=:0.0 ddd %s %d", executable, getpid());
	system(debug_buffer);
}

void lprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void lprintf(const char *fmt, ...) {
	if (log_file) {
		va_list args;

		va_start(args, fmt);
		vfprintf(log_file, fmt, args);
		fflush(log_file);
		va_end(args);
	}
}

@CLASS@

class main_t : public main_window_t {
	public:
		virtual bool process_key(key_t key) {
			if (key == (EKEY_CTRL | 'q'))
				exit_main_loop(EXIT_SUCCESS);
			return main_window_t::process_key(key);
		}
};

static main_window_t *main_window;
static void cleanup(void) {
	delete main_window;
}

int main(int argc, char *argv[]) {
	struct rlimit vm_limit;
	getrlimit(RLIMIT_AS, &vm_limit);
	vm_limit.rlim_cur = 250 * 1024 * 1024;
	setrlimit(RLIMIT_AS, &vm_limit);

	int c;
	while ((c = getopt(argc, argv, "dht")) != -1) {
		switch (c) {
			case 'd':
				getchar();
				break;
			case 'h':
				printf("Usage: test [<options>]\n");
				printf("  -d         Wait for debugger to attach\n");
				printf("  -t         Enable test mode (disable automatic debugger launch\n");
				break;
			case 't':
				option_test_mode = true;
				break;
			default:
				fprintf(stderr, "Unknown option -%c\n", c);
				exit(EXIT_FAILURE);
		}
	}

	if (!option_test_mode) {
		executable = argv[0];
		signal(SIGSEGV, start_debugger_on_segfault);
		signal(SIGABRT, start_debugger_on_segfault);
	}

	setlocale(LC_ALL, "");
	log_file = fopen("test.log", "w+");

	complex_error_t result;
	init_parameters_t *params = init_parameters_t::create();
	if (!(result = init(params)).get_success()) {
		fprintf(stderr, "Error: %s\n", result.get_string());
		fprintf(stderr, "init failed\n");
		exit(EXIT_FAILURE);
	}
	delete params;
	#ifdef CALL_INIT
	CALL_INIT
	#endif

	main_window = new main_t();
	atexit(::cleanup);
	main_window->show();
	set_key_timeout(100);
	main_loop();
	return 0;
}
