#define MAX_IRQS 255

#define DEFAULTS "/etc/ipaq-sleep.conf"
#define APM "/proc/apm"
#define INTERRUPTS "/proc/interrupts"
#define LOADAVG "/proc/loadavg"
#define NO_SLEEP_DIR "/var/run/no-sleep"
#define NO_SLEEP "/var/run/no-sleep/ 2> /dev/null"
#define DEFAULT_SLEEP_TIME 1

#define uflag	"auto-sleep_time"
#define oflag	"dim_time"
#define aflag	"check_apm"
#define cflag	"check_cpu"
#define dflag	"debug"
#define xflag	"X"
#define Cflag	"CPU_value"
#define pflag	"probe_IRQs"
#define iflag	"IRQ"

char *sleep_command="/sbin/pm_helper suspend";
