/*
 * Copyright (c) 2010, Sangoma Technologies
 * Moises Silva <moy@sangoma.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * * Neither the name of the original author; nor the names of any contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 * 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "private/ftdm_core.h"

FTDM_ENUM_NAMES(CHANNEL_STATE_NAMES, CHANNEL_STATE_STRINGS)
FTDM_STR2ENUM(ftdm_str2ftdm_channel_state, ftdm_channel_state2str, ftdm_channel_state_t, CHANNEL_STATE_NAMES, FTDM_CHANNEL_STATE_INVALID)

/* This function is only needed for boost and we should get rid of it at the next refactoring */
FT_DECLARE(ftdm_status_t) ftdm_channel_init(ftdm_channel_t *fchan)
{
	ftdm_channel_lock(fchan);

	if (fchan->init_state != FTDM_CHANNEL_STATE_DOWN) {
		ftdm_channel_set_state(__FILE__, __FUNCTION__, __LINE__, fchan, fchan->init_state, 1);
		fchan->init_state = FTDM_CHANNEL_STATE_DOWN;
	}

	ftdm_channel_unlock(fchan);
	return FTDM_SUCCESS;
}

FT_DECLARE(ftdm_status_t) _ftdm_channel_complete_state(const char *file, const char *func, int line, ftdm_channel_t *fchan)
{
	uint8_t hindex = 0;
	ftdm_channel_state_t state = fchan->state;

	if (fchan->state_status == FTDM_STATE_STATUS_COMPLETED) {
		return FTDM_SUCCESS;
	}

	if (state == FTDM_CHANNEL_STATE_PROGRESS) {
		ftdm_set_flag(fchan, FTDM_CHANNEL_PROGRESS);
	} else if (state == FTDM_CHANNEL_STATE_UP) {
		ftdm_set_flag(fchan, FTDM_CHANNEL_PROGRESS);
		ftdm_set_flag(fchan, FTDM_CHANNEL_MEDIA);	
		ftdm_set_flag(fchan, FTDM_CHANNEL_ANSWERED);	
	} else if (state == FTDM_CHANNEL_STATE_PROGRESS_MEDIA) {
		ftdm_set_flag(fchan, FTDM_CHANNEL_PROGRESS);	
		ftdm_set_flag(fchan, FTDM_CHANNEL_MEDIA);	
	}

	/* if there is a pending ack for an indication */
	if (ftdm_test_flag(fchan, FTDM_CHANNEL_IND_ACK_PENDING)) {
		ftdm_ack_indication(fchan, fchan->indication, FTDM_SUCCESS);
		ftdm_clear_flag(fchan, FTDM_CHANNEL_IND_ACK_PENDING);
	}

	ftdm_log_chan_ex(fchan, file, func, line, FTDM_LOG_LEVEL_DEBUG, "Completed state change from %s to %s\n", ftdm_channel_state2str(fchan->state), ftdm_channel_state2str(state));
	hindex = (fchan->hindex == 0) ? (ftdm_array_len(fchan->history) - 1) : (hindex - 1);
	
	ftdm_assert(!fchan->history[hindex].end_time, "End time should be zero!\n");

	fchan->history[hindex].end_time = ftdm_current_time_in_ms();

	/* FIXME: broadcast condition to wake up anyone waiting on state completion if the channel 
	 * is blocking (FTDM_CHANNEL_NONBLOCK is not set) */

	return FTDM_SUCCESS;
}

FT_DECLARE(ftdm_status_t) _ftdm_set_state(const char *file, const char *func, int line,
		ftdm_channel_t *fchan, ftdm_channel_state_t state)
{
	if (fchan->state_status == FTDM_STATE_STATUS_NEW) {
		/* the current state is new, setting a new state from a signaling module
		   when the current state is new is equivalent to implicitly acknowledging 
		   the current state */
		_ftdm_channel_complete_state(file, func, line, fchan);
	}
	return ftdm_channel_set_state(file, func, line, fchan, state, 0);
}

static int ftdm_parse_state_map(ftdm_channel_t *ftdmchan, ftdm_channel_state_t state, ftdm_state_map_t *state_map)
{
	int x = 0, ok = 0;
	ftdm_state_direction_t direction = ftdm_test_flag(ftdmchan, FTDM_CHANNEL_OUTBOUND) ? ZSD_OUTBOUND : ZSD_INBOUND;

	for(x = 0; x < FTDM_MAP_NODE_SIZE; x++) {
		int i = 0, proceed = 0;
		if (!state_map->nodes[x].type) {
			break;
		}

		if (state_map->nodes[x].direction != direction) {
			continue;
		}
		
		if (state_map->nodes[x].check_states[0] == FTDM_ANY_STATE) {
			proceed = 1;
		} else {
			for(i = 0; i < FTDM_MAP_MAX; i++) {
				if (state_map->nodes[x].check_states[i] == ftdmchan->state) {
					proceed = 1;
					break;
				}
			}
		}

		if (!proceed) {
			continue;
		}
		
		for(i = 0; i < FTDM_MAP_MAX; i++) {
			ok = (state_map->nodes[x].type == ZSM_ACCEPTABLE);
			if (state_map->nodes[x].states[i] == FTDM_END) {
				break;
			}
			if (state_map->nodes[x].states[i] == state) {
				ok = !ok;
				goto end;
			}
		}
	}
 end:
	
	return ok;
}

/* this function MUST be called with the channel lock held. If waitrq == 1, the channel will be unlocked/locked (never call it with waitrq == 1 with an lock recursivity > 1) */
#define DEFAULT_WAIT_TIME 1000
FT_DECLARE(ftdm_status_t) ftdm_channel_set_state(const char *file, const char *func, int line, ftdm_channel_t *ftdmchan, ftdm_channel_state_t state, int waitrq)
{
	int ok = 1;
	int waitms = DEFAULT_WAIT_TIME;	

	if (!ftdm_test_flag(ftdmchan, FTDM_CHANNEL_READY)) {
		ftdm_log_chan_ex(ftdmchan, file, func, line, FTDM_LOG_LEVEL_ERROR, "Ignored state change request from %s to %s, the channel is not ready\n",
				ftdm_channel_state2str(ftdmchan->state), ftdm_channel_state2str(state));
		return FTDM_FAIL;
	}

	if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_STATE_CHANGE)) {
		ftdm_log_chan_ex(ftdmchan, file, func, line, FTDM_LOG_LEVEL_ERROR, "Ignored state change request from %s to %s, the previous state change has not been processed yet\n",
				ftdm_channel_state2str(ftdmchan->state), ftdm_channel_state2str(state));
		return FTDM_FAIL;
	}

	if (ftdmchan->state == state) {
		ftdm_log_chan_ex(ftdmchan, file, func, line, FTDM_LOG_LEVEL_WARNING, "Why bother changing state from %s to %s\n", ftdm_channel_state2str(ftdmchan->state), ftdm_channel_state2str(state));
		return FTDM_FAIL;
	}

	if (ftdmchan->span->state_map) {
		ok = ftdm_parse_state_map(ftdmchan, state, ftdmchan->span->state_map);
		goto end;
	}

	/* basic core state validation (by-passed if the signaling module provides a state_map) */
	switch(ftdmchan->state) {
	case FTDM_CHANNEL_STATE_HANGUP:
	case FTDM_CHANNEL_STATE_TERMINATING:
		{
			ok = 0;
			switch(state) {
			case FTDM_CHANNEL_STATE_DOWN:
			case FTDM_CHANNEL_STATE_BUSY:
			case FTDM_CHANNEL_STATE_RESTART:
				ok = 1;
				break;
			default:
				break;
			}
		}
		break;
	case FTDM_CHANNEL_STATE_UP:
		{
			ok = 1;
			switch(state) {
			case FTDM_CHANNEL_STATE_PROGRESS:
			case FTDM_CHANNEL_STATE_PROGRESS_MEDIA:
			case FTDM_CHANNEL_STATE_RING:
				ok = 0;
				break;
			default:
				break;
			}
		}
		break;
	case FTDM_CHANNEL_STATE_DOWN:
		{
			ok = 0;
			
			switch(state) {
			case FTDM_CHANNEL_STATE_DIALTONE:
			case FTDM_CHANNEL_STATE_COLLECT:
			case FTDM_CHANNEL_STATE_DIALING:
			case FTDM_CHANNEL_STATE_RING:
			case FTDM_CHANNEL_STATE_PROGRESS_MEDIA:
			case FTDM_CHANNEL_STATE_PROGRESS:				
			case FTDM_CHANNEL_STATE_IDLE:				
			case FTDM_CHANNEL_STATE_GET_CALLERID:
			case FTDM_CHANNEL_STATE_GENRING:
				ok = 1;
				break;
			default:
				break;
			}
		}
		break;
	case FTDM_CHANNEL_STATE_BUSY:
		{
			switch(state) {
			case FTDM_CHANNEL_STATE_UP:
				ok = 0;
				break;
			default:
				break;
			}
		}
		break;
	case FTDM_CHANNEL_STATE_RING:
		{
			switch(state) {
			case FTDM_CHANNEL_STATE_UP:
				ok = 1;
				break;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}

end:

	if (ok) {
		ftdm_log_chan_ex(ftdmchan, file, func, line, FTDM_LOG_LEVEL_DEBUG, "Changed state from %s to %s\n", ftdm_channel_state2str(ftdmchan->state), ftdm_channel_state2str(state));
		ftdmchan->last_state = ftdmchan->state; 
		ftdmchan->state = state;
		ftdmchan->state_status = FTDM_STATE_STATUS_NEW;
		ftdmchan->history[ftdmchan->hindex].file = file;
		ftdmchan->history[ftdmchan->hindex].func = func;
		ftdmchan->history[ftdmchan->hindex].line = line;
		ftdmchan->history[ftdmchan->hindex].state = ftdmchan->state;
		ftdmchan->history[ftdmchan->hindex].last_state = ftdmchan->last_state;
		ftdmchan->history[ftdmchan->hindex].time = ftdm_current_time_in_ms();
		ftdmchan->history[ftdmchan->hindex].end_time = 0;
		ftdmchan->hindex++;
		if (ftdmchan->hindex == ftdm_array_len(ftdmchan->history)) {
			ftdmchan->hindex = 0;
		}
		ftdm_set_flag(ftdmchan, FTDM_CHANNEL_STATE_CHANGE);

		ftdm_mutex_lock(ftdmchan->span->mutex);
		ftdm_set_flag(ftdmchan->span, FTDM_SPAN_STATE_CHANGE);
		if (ftdmchan->span->pendingchans) {
			ftdm_queue_enqueue(ftdmchan->span->pendingchans, ftdmchan);
		}
		ftdm_mutex_unlock(ftdmchan->span->mutex);
	} else {
		ftdm_log_chan_ex(ftdmchan, file, func, line, FTDM_LOG_LEVEL_WARNING, "VETO state change from %s to %s\n", ftdm_channel_state2str(ftdmchan->state), ftdm_channel_state2str(state));
		goto done;
	}

	if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_NONBLOCK)) {
		/* the channel should not block waiting for state processing */
		goto done;
	}

	/* there is an inherent race here between set and check of the change flag but we do not care because
	 * the flag should never last raised for more than a few ms for any state change */
	while (waitrq && waitms > 0) {
		/* give a chance to the signaling stack to process it */
		ftdm_mutex_unlock(ftdmchan->mutex);

		ftdm_sleep(10);
		waitms -= 10;

		ftdm_mutex_lock(ftdmchan->mutex);
		
		/* if the flag is no longer set, the state change was processed (or is being processed) */
		if (!ftdm_test_flag(ftdmchan, FTDM_CHANNEL_STATE_CHANGE)) {
			break;
		}

		/* if the state is no longer what we set, the state change was 
		 * obviously processed (and the current state change flag is for other state change) */
		if (ftdmchan->state != state) {
			break;
		}
	}

	if (waitms <= 0) {
		ftdm_log_chan_ex(ftdmchan, file, func, line, FTDM_LOG_LEVEL_WARNING, "state change from %s to %s was most likely not processed after aprox %dms\n",
				ftdm_channel_state2str(ftdmchan->last_state), ftdm_channel_state2str(state), DEFAULT_WAIT_TIME);
		ok = 0;
	}
done:
	return ok ? FTDM_SUCCESS : FTDM_FAIL;
}

FT_DECLARE(int) ftdm_channel_get_state(const ftdm_channel_t *ftdmchan)
{
	int state;
	ftdm_channel_lock(ftdmchan);
	state = ftdmchan->state;
	ftdm_channel_unlock(ftdmchan);
	return state;
}

FT_DECLARE(const char *) ftdm_channel_get_state_str(const ftdm_channel_t *ftdmchan)
{
	const char *state;
	ftdm_channel_lock(ftdmchan);
	state = ftdm_channel_state2str(ftdmchan->state);
	ftdm_channel_unlock(ftdmchan);
	return state;
}

FT_DECLARE(int) ftdm_channel_get_last_state(const ftdm_channel_t *ftdmchan)
{
	int last_state;
	ftdm_channel_lock(ftdmchan);
	last_state = ftdmchan->last_state;
	ftdm_channel_unlock(ftdmchan);
	return last_state;
}

FT_DECLARE(const char *) ftdm_channel_get_last_state_str(const ftdm_channel_t *ftdmchan)
{
	const char *state;
	ftdm_channel_lock(ftdmchan);
	state = ftdm_channel_state2str(ftdmchan->last_state);
	ftdm_channel_unlock(ftdmchan);
	return state;
}

static void write_history_entry(const ftdm_channel_t *fchan, ftdm_stream_handle_t *stream, int i, ftdm_time_t *prevtime)
{
	char func[255];
	char line[255];
	char states[255];
	const char *filename = NULL;
	snprintf(states, sizeof(states), "%-5.15s => %-5.15s", ftdm_channel_state2str(fchan->history[i].last_state), ftdm_channel_state2str(fchan->history[i].state));
	snprintf(func, sizeof(func), "[%s]", fchan->history[i].func);
	filename = strrchr(fchan->history[i].file, *FTDM_PATH_SEPARATOR);
	if (!filename) {
		filename = fchan->history[i].file;
	} else {
		filename++;
	}
	if (!(*prevtime)) {
		*prevtime = fchan->history[i].time;
	}
	snprintf(line, sizeof(func), "[%s:%d]", filename, fchan->history[i].line);
	stream->write_function(stream, "%-30.30s %-30.30s %-30.30s %lums\n", states, func, line, (fchan->history[i].time - *prevtime));
	*prevtime = fchan->history[i].time;
}

FT_DECLARE(char *) ftdm_channel_get_history_str(const ftdm_channel_t *fchan)
{
	uint8_t i = 0;
	ftdm_time_t currtime = 0;
	ftdm_time_t prevtime = 0;

	ftdm_stream_handle_t stream = { 0 };
	FTDM_STANDARD_STREAM(stream);
	if (!fchan->history[0].file) {
		stream.write_function(&stream, "-- No state history --\n");
		return stream.data;
	}

	stream.write_function(&stream, "%-30.30s %-30.30s %-30.30s %s", 
			"-- States --", "-- Function --", "-- Location --", "-- Time Offset --\n");

	for (i = fchan->hindex; i < ftdm_array_len(fchan->history); i++) {
		if (!fchan->history[i].file) {
			break;
		}
		write_history_entry(fchan, &stream, i, &prevtime);
	}

	for (i = 0; i < fchan->hindex; i++) {
		write_history_entry(fchan, &stream, i, &prevtime);
	}

	currtime = ftdm_current_time_in_ms();

	stream.write_function(&stream, "\nTime since last state change: %lums\n", (currtime - prevtime));

	return stream.data;
}

FT_DECLARE(ftdm_status_t) ftdm_channel_advance_states(ftdm_channel_t *fchan, ftdm_channel_state_processor_t state_processor)
{
	ftdm_channel_state_t state;
	
	ftdm_channel_lock(fchan);
	while (fchan->state_status == FTDM_STATE_STATUS_NEW) {
		state = fchan->state;
		state_processor(fchan);
		if (state == fchan->state) {
			/* if the state did not change, the state status must go to PROCESSED
			 * otherwise we don't touch it since is a new state and the old state was
			 * already completed implicitly by the state_processor() function via some internal
			 * call to ftdm_set_state() */
			fchan->state_status = FTDM_STATE_STATUS_PROCESSED;
		}
	}
	ftdm_channel_unlock(fchan);
	return FTDM_SUCCESS;
}

FT_DECLARE(int) ftdm_check_state_all(ftdm_span_t *span, ftdm_channel_state_t state)
{
	uint32_t j;
	for(j = 1; j <= span->chan_count; j++) {
		if (span->channels[j]->state != state || ftdm_test_flag(span->channels[j], FTDM_CHANNEL_STATE_CHANGE)) {
			return 0;
		}
	}
	return 1;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */
