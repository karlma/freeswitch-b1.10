#include <switch.h>

struct switch_scheduler_task_container {
	switch_scheduler_task_t task;
	time_t executed;
	int in_thread;
	int destroyed;
	switch_scheduler_func_t func;
	switch_memory_pool_t *pool;
	uint32_t flags;
	char *desc;
	struct switch_scheduler_task_container *next;
};
typedef struct switch_scheduler_task_container switch_scheduler_task_container_t;


static struct {
	switch_scheduler_task_container_t *task_list;
	switch_mutex_t *task_mutex;
	uint32_t task_id;
	int task_thread_running;
	switch_memory_pool_t *memory_pool;
} globals;

static void switch_scheduler_execute(switch_scheduler_task_container_t *tp)
{
	switch_event_t *event;
	//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Executing task %u %s (%s)\n", tp->task.task_id, tp->desc, switch_str_nil(tp->task.group));

	tp->func(&tp->task);

	if (tp->task.runtime > tp->executed) {
		tp->executed = 0;
		if (switch_event_create(&event, SWITCH_EVENT_RE_SCHEDULE) == SWITCH_STATUS_SUCCESS) {
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-ID", "%u", tp->task.task_id);
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-Desc", "%s", tp->desc);
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-Group", "%s", switch_str_nil(tp->task.group));
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-Runtime", "%" TIME_T_FMT, tp->task.runtime);
			switch_event_fire(&event);
		}
	} else {
		if (switch_event_create(&event, SWITCH_EVENT_DEL_SCHEDULE) == SWITCH_STATUS_SUCCESS) {
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-ID", "%u", tp->task.task_id);
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-Desc", "%s", tp->desc);
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-Group", "%s", switch_str_nil(tp->task.group));
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-Runtime", "%" TIME_T_FMT, tp->task.runtime);
			switch_event_fire(&event);
		}
		tp->destroyed = 1;
	}
}

static void *SWITCH_THREAD_FUNC task_own_thread(switch_thread_t *thread, void *obj)
{
	switch_scheduler_task_container_t *tp = (switch_scheduler_task_container_t *) obj;
	switch_memory_pool_t *pool;

	pool = tp->pool;
	tp->pool = NULL;

	switch_scheduler_execute(tp);
	switch_core_destroy_memory_pool(&pool);
	tp->in_thread = 0;

	return NULL;
}

static int task_thread_loop(int done)
{
	switch_scheduler_task_container_t *tofree, *tp, *last = NULL;


	switch_mutex_lock(globals.task_mutex);
	for (tp = globals.task_list; tp; tp = tp->next) {
		if (done) {
			tp->destroyed = 1;
		} else {
			time_t now = time(NULL);
			if (now >= tp->task.runtime && !tp->in_thread) {
				tp->executed = now;
				if (switch_test_flag(tp, SSHF_OWN_THREAD)) {
					switch_thread_t *thread;
					switch_threadattr_t *thd_attr;
					switch_core_new_memory_pool(&tp->pool);
					switch_threadattr_create(&thd_attr, tp->pool);
					switch_threadattr_detach_set(thd_attr, 1);
					tp->in_thread = 1;
					switch_thread_create(&thread, thd_attr, task_own_thread, tp, tp->pool);
				} else {
					switch_scheduler_execute(tp);
				}
			}
		}
	}
	switch_mutex_unlock(globals.task_mutex);
	switch_mutex_lock(globals.task_mutex);
	for (tp = globals.task_list; tp;) {
		if (tp->destroyed && !tp->in_thread) {
			tofree = tp;
			tp = tp->next;
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Deleting task %u %s (%s)\n",
							  tofree->task.task_id, tofree->desc, switch_str_nil(tofree->task.group));
			if (last) {
				last->next = tofree->next;
			} else {
				globals.task_list = tofree->next;
			}
			switch_safe_free(tofree->task.group);
			if (tofree->task.cmd_arg && switch_test_flag(tofree, SSHF_FREE_ARG)) {
				free(tofree->task.cmd_arg);
			}
			switch_safe_free(tofree->desc);
			free(tofree);
		} else {
			last = tp;
			tp = tp->next;
		}
	}
	switch_mutex_unlock(globals.task_mutex);

	return done;
}

static void *SWITCH_THREAD_FUNC switch_scheduler_task_thread(switch_thread_t *thread, void *obj)
{

	globals.task_thread_running = 1;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Starting task thread\n");
	while (globals.task_thread_running == 1) {
		if (task_thread_loop(0)) {
			break;
		}
		switch_yield(500000);
	}

	task_thread_loop(1);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Task thread ending\n");
	globals.task_thread_running = 0;

	return NULL;
}

SWITCH_DECLARE(uint32_t) switch_scheduler_add_task(time_t task_runtime,
												   switch_scheduler_func_t func,
												   char *desc,
												   char *group,
												   uint32_t cmd_id, void *cmd_arg, switch_scheduler_flag_t flags)
{
	switch_scheduler_task_container_t *container, *tp;
	switch_event_t *event;

	switch_mutex_lock(globals.task_mutex);
	switch_zmalloc(container, sizeof(*container));
	assert(func);
	container->func = func;
	time(&container->task.created);
	container->task.runtime = task_runtime;
	container->task.group = strdup(group ? group : "none");
	container->task.cmd_id = cmd_id;
	container->task.cmd_arg = cmd_arg;
	container->flags = flags;
	container->desc = strdup(desc ? desc : "none");

	for (tp = globals.task_list; tp && tp->next; tp = tp->next);

	if (tp) {
		tp->next = container;
	} else {
		globals.task_list = container;
	}

	for (container->task.task_id = 0; !container->task.task_id; container->task.task_id = ++globals.task_id);

	switch_mutex_unlock(globals.task_mutex);

	tp = container;
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Added task %u %s (%s) to run at %" TIME_T_FMT "\n",
					  tp->task.task_id, tp->desc, switch_str_nil(tp->task.group), task_runtime);

	if (switch_event_create(&event, SWITCH_EVENT_ADD_SCHEDULE) == SWITCH_STATUS_SUCCESS) {
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-ID", "%u", tp->task.task_id);
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-Desc", "%s", tp->desc);
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-Group", "%s", switch_str_nil(tp->task.group));
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-Runtime", "%" TIME_T_FMT, tp->task.runtime);
		switch_event_fire(&event);
	}
	return container->task.task_id;
}

SWITCH_DECLARE(switch_status_t) switch_scheduler_del_task_id(uint32_t task_id)
{
	switch_scheduler_task_container_t *tp;
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *event;

	switch_mutex_lock(globals.task_mutex);
	for (tp = globals.task_list; tp; tp = tp->next) {
		if (tp->task.task_id == task_id) {
			tp->destroyed++;
			if (switch_event_create(&event, SWITCH_EVENT_DEL_SCHEDULE) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-ID", "%u", tp->task.task_id);
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-Desc", "%s", tp->desc);
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-Group", "%s", switch_str_nil(tp->task.group));
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-Runtime", "%" TIME_T_FMT, tp->task.runtime);
				switch_event_fire(&event);
			}
			status = SWITCH_STATUS_SUCCESS;
			break;
		}
	}
	switch_mutex_unlock(globals.task_mutex);

	return status;
}

SWITCH_DECLARE(switch_status_t) switch_scheduler_del_task_group(char *group)
{
	switch_scheduler_task_container_t *tp;
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *event;

	switch_mutex_lock(globals.task_mutex);
	for (tp = globals.task_list; tp; tp = tp->next) {
		if (!strcmp(tp->task.group, group)) {
			if (switch_event_create(&event, SWITCH_EVENT_DEL_SCHEDULE) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-ID", "%u", tp->task.task_id);
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-Desc", "%s", tp->desc);
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-Group", "%s", switch_str_nil(tp->task.group));
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Task-Runtime", "%" TIME_T_FMT, tp->task.runtime);
				switch_event_fire(&event);
			}
			tp->destroyed++;
			status = SWITCH_STATUS_SUCCESS;
		}
	}
	switch_mutex_unlock(globals.task_mutex);

	return status;
}

SWITCH_DECLARE(void) switch_scheduler_task_thread_start(void)
{
	switch_thread_t *thread;
	switch_threadattr_t *thd_attr;

	switch_core_new_memory_pool(&globals.memory_pool);
	switch_threadattr_create(&thd_attr, globals.memory_pool);
	switch_mutex_init(&globals.task_mutex, SWITCH_MUTEX_NESTED, globals.memory_pool);

	switch_threadattr_detach_set(thd_attr, 1);
	switch_thread_create(&thread, thd_attr, switch_scheduler_task_thread, NULL, globals.memory_pool);
}


SWITCH_DECLARE(void) switch_scheduler_task_thread_stop(void)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Stopping Task Thread\n");
	if (globals.task_thread_running == 1) {
		int sanity = 0;

		globals.task_thread_running = -1;

		while (globals.task_thread_running) {
			switch_yield(100000);
			if (++sanity > 10) {
				break;
			}
		}
	}
}
