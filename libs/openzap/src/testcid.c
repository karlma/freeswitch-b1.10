#include "openzap.h"

int main(int argc, char *argv[])
{
	zap_fsk_data_state_t state = {0};
	int fd;
	int16_t buf[160] = {0};
	ssize_t len = 0;
	int type, mlen;
	char *sp;
	char str[128] = "";
	char fbuf[256];

	if (argc < 2 || zap_fsk_demod_init(&state, 8000, fbuf, sizeof(fbuf))) {
		printf("wtf\n");
		return 0;
	}

	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "cant open file %s\n", argv[1]);
		exit (-1);
	}

	while((len = read(fd, buf, sizeof(buf))) > 0) {
		if (zap_fsk_demod_feed(&state, buf, len / 2) != ZAP_SUCCESS) {
			break;
		}
	}

	while(zap_fsk_data_parse(&state, &type, &sp, &mlen) == ZAP_SUCCESS) {
		zap_copy_string(str, sp, mlen+1);
		*(str+mlen) = '\0';
		zap_clean_string(str);
		printf("TYPE %d (%s) LEN %d VAL [%s]\n", type, zap_mdmf_type2str(type), mlen, str);
	}

	zap_fsk_demod_destroy(&state);

	close(fd);
}
