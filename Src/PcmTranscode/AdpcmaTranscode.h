#ifndef _ADPCM_H_
#define _ADPCM_H_

typedef struct adpcm_state_t {
		short	valprev;	/* Previous output value */
		char	index;		/* Index into stepsize table */
}adpcm_state;


void adpcm_coder(short *indata, char *outdata, int len, adpcm_state *state);

void adpcm_decoder(char *indata, short *outdata, int len, adpcm_state *state);


#endif

