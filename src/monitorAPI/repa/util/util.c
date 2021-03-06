/*
 * util.c
 *
 *  Created on: 26/07/2011
 *      Author: Héberte Fernandes de Moraes
 */

#include <time.h>
#include <math.h>
#include <string.h> /* for str* and mem* functions */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <ctype.h>
#include <dirent.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <syslog.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <linux/if_ether.h>

#ifdef ANDROID
#include <android/log.h>
#endif

#include "hdr/linkedlist.h"
#include "hdr/util.h"
#include "hdr/repa.h"

/* Text that represent the message types */
const char* TMSG_types_messages[] = {
	"Do Nothing",
	"Send Message",
	"Register Interest",
	"Unregister Interest",
	"Ask Node Address",
	"Set up Ad-hoc network",
	"Set down Ad-hoc network",
	"Ask Ad-hoc is actived",
	"Receive Message",
	"Get list of Interest",
	"Send Timestamp",
	"Send Ping",
	"Clear Statistics",
	"Get list of Nodes",
	"Create Ad-hoc network in wpa_supplicant",
	"Send hide Message",
	"Get Statistics"
};


void repa_print_interests(struct dllist *list_interests) {
	int64_t now, diff;
	struct dll_node *aux;
	struct interest *interest;

	if (list_interests == NULL) {
		repad_printf(LOG_INFO, "No interest in network\n");
		return;
	}

	now = get_nanoseconds();

	repad_printf(LOG_INFO, "Interest in network: %d\n", list_interests->num_elements);
	for (aux = list_interests->head; aux != NULL; aux = aux->next) {
		interest = (struct interest*)aux->data;
		diff = now - interest->timestamp;
		repad_printf(LOG_INFO, "\t%s\t\t%lldns\n", interest->interest, diff);
	}
}

void repa_print_nodes(struct dllist *list_nodes) {
	int64_t now, diff;
	char prefix[20];
	struct node *node;
	struct dll_node *aux;

	if (list_nodes == NULL) {
		repad_printf(LOG_INFO, "No node in network\n");
		return;
	}

	now = get_nanoseconds();

	repad_printf(LOG_INFO, "Nodes in network: %d\n", list_nodes->num_elements);
	for (aux = list_nodes->head; aux != NULL; aux = aux->next) {
		node = (struct node*)aux->data;
		diff = now - node->timestamp;
		repa_print_prefix(node->prefix, prefix);
		repad_printf(LOG_INFO, "\t%s -> %d\t\t%lldns\n", prefix, node->prefix, diff);
	}
}

void repa_print_statistics(bool verbose, uint32_t address, statistics_t *statistics) {
	char prefix[50];

	if (statistics != NULL) {
		repa_print_prefix(address, prefix);
		if (verbose) {
			repad_printf(LOG_INFO, "Statistics %s:\n", prefix);

			repad_printf(LOG_INFO, "\tMessage Unique Receive: %d\n", statistics->qtyMessageUniqueReceive);
			repad_printf(LOG_INFO, "\tMessage Total: %d\n", statistics->qtyMessageTotalReceived);
			repad_printf(LOG_INFO, "\tMessage Accept Interest: %d\n", statistics->qtyMessageAcceptInterest);

			repad_printf(LOG_INFO, "\tMessage Dropped Prefix: %d\n", statistics->qtyMessageDroppedPrefixP);
			repad_printf(LOG_INFO, "\tMessage Dropped Max HTL: %d\n", statistics->qtyMessageDroppedMaxHTL);
			repad_printf(LOG_INFO, "\tMessage Dropped Memory: %d\n", statistics->qtyMessageDroppedMemory);

			repad_printf(LOG_INFO, "\tMessage Sent: %d\n", statistics->qtyMessageSent);
			repad_printf(LOG_INFO, "\tMessage Sent Collaborative: %d\n", statistics->qtyMessageSentCollaborative);

			repad_printf(LOG_INFO, "\tMean HTL: %d\n", statistics->meanHtl);
			repad_printf(LOG_INFO, "\tMean Latency: %lldns\n", statistics->meanLatency);
			repad_printf(LOG_INFO, "\tMean Propagation Time: %lld\n", statistics->meanPropagationTime);

			repad_printf(LOG_INFO, "\tMean HTL for all messages: %d\n", statistics->meanHtlForAll);
		} else {
			repad_printf(LOG_INFO, "%s %d %d %d %d %d %d %d %d %d %ld %d \n", prefix,
			statistics->qtyMessageUniqueReceive, statistics->qtyMessageTotalReceived, statistics->qtyMessageAcceptInterest,
			statistics->qtyMessageDroppedPrefixP, statistics->qtyMessageDroppedMaxHTL, statistics->qtyMessageDroppedMemory,
			statistics->qtyMessageSent, statistics->qtyMessageSentCollaborative, statistics->meanHtl, statistics->meanLatency,
			statistics->meanHtlForAll);
		}
	}
}

/* Normal random variable generator */
float rand_gauss(float m, float s) { /* mean m, standard deviation s */
	float x1, x2, w, y1;
	static float y2;
	static int use_last = 0;

	if (use_last) { /* use value from previous call */
		y1 = y2;
		use_last = 0;
	} else {
		do {
			x1 = 2.0 * (rand()/(float)RAND_MAX) - 1.0;
			x2 = 2.0 * (rand()/(float)RAND_MAX) - 1.0;
			w = x1 * x1 + x2 * x2;
		} while ( w >= 1.0 );
		w = sqrt((-2.0 * log(w))/w);
		y1 = x1 * w;
		y2 = x2 * w;
		use_last = 1;
	}

	return (m + y1*s);
}

/* Return 1 if the difference is negative, otherwise 0.  */
long int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1) {
	long int diff = (t2->tv_usec + USEC_PER_SEC*t2->tv_sec)
			- (t1->tv_usec + USEC_PER_SEC*t1->tv_sec);
	result->tv_sec = diff / USEC_PER_SEC;
	result->tv_usec = diff % USEC_PER_SEC;

	return (diff<0);
}

#define READ_BUF_SIZE	50
pid_t* find_pid_by_name(const char* pidName) {
	int i = 0;
	DIR *dir = NULL;
	struct dirent *next;
	pid_t* pidList = NULL;

	if ((dir = opendir("/proc")) == NULL) {
		return NULL;
	}

	while ((next = readdir(dir)) != NULL) {
		FILE *status;
		char filename[READ_BUF_SIZE];
		char buffer[READ_BUF_SIZE];
		char name[READ_BUF_SIZE];

		/* Must skip ".." since that is outside /proc */
		if (strcmp(next->d_name, "..") == 0)
			continue;

		/* If it isn't a number, we don't want it */
		if (!isdigit(*next->d_name))
			continue;

		sprintf(filename, "/proc/%s/status", next->d_name);
		if (! (status = fopen(filename, "r")) ) {
			continue;
		}
		if (fgets(buffer, READ_BUF_SIZE-1, status) == NULL) {
			fclose(status);
			continue;
		}
		fclose(status);

		/* Buffer should contain a string like "Name:   binary_name" */
		sscanf(buffer, "%*s %s", name);
		if (strncmp(name, pidName, MIN(strlen(name), strlen(pidName))) == 0) {
			if (pidList == NULL) {
				pidList = malloc(sizeof(pid_t));
			} else {
				pidList = realloc(pidList, sizeof(pid_t) * (i+1));
			}
			pidList[i++] = strtol(next->d_name, NULL, 0);
		}
	}

	if (dir != NULL) {
		closedir(dir);
	}

	if (pidList) {
		pidList[i] = 0;
	} else if (strcmp(pidName, "init") == 0) {
		/* If we found nothing and they were trying to kill "init",
		 * guess PID 1 and call it good...  Perhaps we should simply
		 * exit if /proc isn't mounted, but this will do for now. */
		pidList = realloc(pidList, sizeof(pid_t));
		pidList[0] = 1;
	} else {
		pidList = realloc(pidList, sizeof(pid_t));
		pidList[0] = -1;
	}
	return pidList;
}

#ifdef USE_LOG
static FILE *repad_out = NULL;
#endif

int open_repad_out(bool terminal, const char* repad_log_file) {
#ifdef USE_LOG
	int ret = 0;

	if (terminal) {
		repad_out = stdout;
	} else {
		if (repad_log_file != NULL) {
			repad_out = fopen(repad_log_file, "w+");
		}
	}
	ret = (repad_out != NULL);

#ifdef SYSLOG
#	ifndef ANDROID
     openlog (repa_application_name, LOG_PID, LOG_DAEMON);
#	endif
#endif

	return ret;
#else
	return 0;
#endif
}

int close_repad_out() {
#ifdef USE_LOG
	int ret = 0;

	if (repad_out != NULL) {
		ret = fclose(repad_out);
	}

#ifdef SYSLOG
#	ifndef ANDROID
	closelog();
#	endif
#endif

	return ret;
#else
	return 0;
#endif
}

inline int64_t get_microseconds() {
	struct timeval time;
	gettimeofday(&time, NULL);
	return (int64_t)(((int64_t)time.tv_sec*USEC_PER_SEC)
			+ (int64_t)time.tv_usec); // Convert to microseconds
}

inline int64_t get_nanoseconds() {
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	return (int64_t)(((int64_t)time.tv_sec*NSEC_PER_SEC)
			+ (int64_t)time.tv_nsec); // Convert to nanoseconds
}

inline void get_formated_time(size_t size, char *time_formated) {
	struct timeval tv;
	struct timezone tz;
	struct tm * timeinfo;

	gettimeofday(&tv,&tz);
	timeinfo = localtime (&tv.tv_sec);
	sprintf(time_formated, "%02d:%02d:%02d.%06ldus", timeinfo->tm_hour,
			timeinfo->tm_min, timeinfo->tm_sec, tv.tv_usec);
}

int repad_printf(int priority, const char *format, ...) {
#ifdef USE_LOG
	char new_format[255], time_formated[50];
	va_list ap;
	int result = 0;
	if (repad_out != NULL) {
		get_formated_time(50, time_formated);
		sprintf(new_format, "[%s] %s", time_formated, format);

		switch (priority) {
			case LOG_INFO:
			case LOG_ERR:
			case LOG_ALERT:
			case LOG_WARNING:
				va_start (ap, format);
				result = vfprintf(repad_out, new_format, ap);
				fflush(repad_out);
				va_end (ap);
				break;
			case LOG_DEBUG:
#ifdef DEBUG
				va_start (ap, format);
				result = vfprintf(repad_out, new_format, ap);
				fflush(repad_out);
				va_end (ap);
#endif
				break;
			default:
				break;
		}
	}
#ifdef SYSLOG
#	ifdef ANDROID
#		ifdef DEBUG
	if (priority != LOG_DEBUG) {
#		endif
		va_start (ap, format);
		if (priority == LOG_INFO) __android_log_vprint(ANDROID_LOG_INFO, repa_application_name, format, ap);
		if (priority == LOG_DEBUG) __android_log_vprint(ANDROID_LOG_DEBUG, repa_application_name, format, ap);
		va_end (ap);
#		ifdef DEBUG
	}
#		endif

#	else
	va_start (ap, format);
	vsyslog(priority, format, ap);
	va_end (ap);
#	endif
#endif
	return result;
#else
	return 0;
#endif
}

void repad_print_mem(const char *data, size_t data_len) {
#if defined(DEBUG) && defined(USE_LOG)
	if (repad_out != NULL) {
		register uint32_t i;
		for (i = 0; i < data_len-1; ++i) {
			fprintf(repad_out,"%2X ", data[i]);
		}
		fprintf(repad_out,"%2X\n", data[i]);
		fflush(repad_out);
	}
#endif
}

void repad_print_char(const char *data, size_t data_len) {
#if defined(DEBUG) && defined(USE_LOG)
	if (repad_out != NULL) {
		register uint32_t i;
		for (i = 0; i < data_len-1; ++i) {
			if (data[i] > 32) {
				fprintf(repad_out,"%c", data[i]);
			} else {
				fprintf(repad_out,"[%d]", data[i]);
			}
		}
		if (data[i] > 32) {
			fprintf(repad_out,"%c\n", data[i]);
		} else {
			fprintf(repad_out,"[%d]\n", data[i]);
		}
		fflush(repad_out);
	}
#endif
}

// Print prefix C
void repa_print_prefix(uint32_t address, char *prefix_result) {
	register uint32_t i;

	sprintf(prefix_result, "[%u", REPA_GET_FIELD(address, REPA_PREFIX_LENGTH, 0, REPA_FIELD_LENGTH));
	for(i = 1; i < REPA_PREFIX_LENGTH; i++) {
		sprintf(prefix_result, "%s#%u", prefix_result, REPA_GET_FIELD(address, REPA_PREFIX_LENGTH, i, REPA_FIELD_LENGTH));
	}
	sprintf(prefix_result, "%s]", prefix_result);
}

// Randomize prefix C
void repa_random_prefix(prefix_addr_t *address) {
	uint32_t i;
	uint32_t auxVal;

	double possibilities = pow(REPA_FIELD_LENGTH, 2);
	double mean = possibilities/2;
	double variance = possibilities/2;

	*address = 0;
	for(i = 0; i < REPA_PREFIX_LENGTH; i++) {
		auxVal = (int)trunc(rand_gauss(mean, variance));
		*address |= (auxVal & REPA_MASK_FIELD(8*sizeof(*address), REPA_PREFIX_LENGTH,0,REPA_FIELD_LENGTH)) << (i*(8*sizeof(*address)/REPA_PREFIX_LENGTH));
	}
}

/*
 * Serialize and Deserialize message
 */
inline void __msg_serialize(char *out_buffer, size_t *out_buffer_size, const prefix_addr_t in_dst_prefix_addr,
		const prefix_addr_t in_src_prefix_addr, const uint8_t in_interest_len, const char *in_interest,
		const uint16_t in_data_len, const char *in_data) {
	uint8_t interest_len = in_interest_len;
	uint16_t data_len = in_data_len;
	if (data_len == 0) {
		data_len = strlen(in_data);
	}

	memcpy(out_buffer, &in_dst_prefix_addr, sizeof(prefix_addr_t)); /* Put destination prefix address */
	memcpy(out_buffer+sizeof(prefix_addr_t), &in_src_prefix_addr, sizeof(prefix_addr_t)); /* Put source prefix address */
	memcpy(out_buffer+2*sizeof(prefix_addr_t), &interest_len, sizeof(uint8_t)); /* Put size of interest */
	memcpy(out_buffer+2*sizeof(prefix_addr_t)+sizeof(uint8_t), in_interest, interest_len); /* Put interest in buffer */
	memcpy(out_buffer+2*sizeof(prefix_addr_t)+sizeof(uint8_t)+interest_len, &data_len, sizeof(uint16_t)); /* Put data length in buffer */
	memcpy(out_buffer+2*sizeof(prefix_addr_t)+sizeof(uint8_t)+interest_len+sizeof(uint16_t), in_data, data_len); /* Put data in buffer */

	(*out_buffer_size) = (2*sizeof(prefix_addr_t)+sizeof(uint8_t)+interest_len+sizeof(uint16_t)+data_len); /* New size of data */
}

inline void __msg_deserialize(const char *in_data, prefix_addr_t *out_dst_prefix_addr,
		prefix_addr_t *out_src_prefix_addr, uint8_t *out_interest_len, char *out_interest,
		uint16_t *out_data_len, char *out_data) {
	if (in_data == NULL) { // No data to read
		return;
	}

	memcpy(out_dst_prefix_addr, in_data, sizeof(prefix_addr_t)); /* Get destination prefix address */
	memcpy(out_src_prefix_addr, in_data + sizeof(prefix_addr_t), sizeof(prefix_addr_t)); /* Get source prefix address */
	memcpy(out_interest_len,	in_data+2*sizeof(prefix_addr_t), sizeof(uint8_t)); /* Get size of interest */
	memcpy(out_interest, 		in_data+2*sizeof(prefix_addr_t)+sizeof(uint8_t), (*out_interest_len)); /* Get interest in buffer */
	memcpy(out_data_len, 		in_data+2*sizeof(prefix_addr_t)+sizeof(uint8_t)+(*out_interest_len), sizeof(uint16_t)); /* Get data length in buffer */
	memcpy(out_data, 			in_data+2*sizeof(prefix_addr_t)+sizeof(uint8_t)+(*out_interest_len)+sizeof(uint16_t), (*out_data_len)); /* Get data in buffer */

	//	repad_print(LOG_INFO, "msg_deserialize: %d %d %d %p %p \"%s\" \"%s\"\n", (*out_dst_addr), (*out_interest_len), (*out_data_len), out_interest, out_data, out_interest, out_data);
}


void print_mem(FILE *where, char *data, size_t data_len) {
	size_t i;
	fprintf(where,"\n-------------------------\n<");
	for (i = 0; i < data_len-1; ++i) {
		fprintf(where,"%2X ", data[i]);
	}
	fprintf(where,"%X", data[i]);
	fprintf(where,">\n-------------------------\n");
}


//#define MAC_FORMAT "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X"
//#define MAC_BYTES(p)  p[0],p[1],p[2],p[3],p[4],p[5]
//
//int main(int argc, char **argv) {
//	int s;
//	struct ifreq buffer;
//	struct ether_addr *mac;
//	struct in_addr ip;
//	uint32_t t;
//
//	s = socket(PF_INET, SOCK_DGRAM, 0);
//	memset(&buffer, 0x00, sizeof(buffer));
//	strcpy(buffer.ifr_name, "eth0");
//	ioctl(s, SIOCGIFHWADDR, &buffer);
//	close(s);
//
//	mac = (struct ether_addr*)buffer.ifr_hwaddr.sa_data;
//	printf(MAC_FORMAT"\n", MAC_BYTES(mac->ether_addr_octet));
//
//	ip.s_addr = generate_ipaddr(mac);
//	printf("picked address: %s \n", inet_ntoa(ip));
//	ip.s_addr = htonl(IPV4_NETMASK);
//	printf("netmask: %s \n", inet_ntoa(ip));
////	printf("netmask: %d.%d.%d.%d\n",
////			htonl(IPV4_NETMASK) & 0xff,
////            ((htonl(IPV4_NETMASK) >> 8) & 0xff),
////            ((htonl(IPV4_NETMASK) >> 16) & 0xff),
////            ((htonl(IPV4_NETMASK) >> 24) & 0xff));
//
//	return 0;
//}

