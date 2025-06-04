/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * This is a sample demonstration application that showcases usage of remoteproc
 * and rpmsg APIs on the remote core. This application is meant to run on the remote CPU
 * running baremetal code. This applicationr receives two matrices from the host,
 * multiplies them and returns the result to the host core.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openamp/open_amp.h>
#include "matrix_multiply.h"
#include "platform_info.h"
#include "FreeRTOS.h"
#include "task.h"
#include "kernel/dpl/DebugP.h"
#include "kernel/dpl/ClockP.h"
#include "kernel/dpl/ti_power_clock_config.h"


#define MAIN_TASK_PRI  (configMAX_PRIORITIES-1)
#define MAIN_TASK_SIZE (32* 1024)
StackType_t gMainTaskStack[MAIN_TASK_SIZE] __attribute__((aligned(32)));

StaticTask_t gMainTaskObj;
TaskHandle_t gMainTask;

#define	MAX_SIZE		6
#define NUM_MATRIX		2

#define SHUTDOWN_MSG	0xEF56A55A

#define LPRINTF(format, ...) printf(format, ##__VA_ARGS__)
//#define LPRINTF(format, ...)
#define LPERROR(format, ...) LPRINTF("ERROR: " format, ##__VA_ARGS__)

typedef struct _matrix {
	unsigned int size;
	unsigned int elements[MAX_SIZE][MAX_SIZE];
} matrix;

/* Local variables */
static struct rpmsg_endpoint lept;
static int shutdown_req = 0;

/*-----------------------------------------------------------------------------*
 *  Calculate the Matrix
 *-----------------------------------------------------------------------------*/
static void Matrix_Multiply(const matrix *m, const matrix *n, matrix *r)
{
	unsigned int i, j, k;

	memset(r, 0x0, sizeof(matrix));
	r->size = m->size;

	for (i = 0; i < m->size; ++i) {
		for (j = 0; j < n->size; ++j) {
			for (k = 0; k < r->size; ++k) {
				r->elements[i][j] +=
					m->elements[i][k] * n->elements[k][j];
			}
		}
	}
}

/*-----------------------------------------------------------------------------*
 *  RPMSG callbacks setup by remoteproc_resource_init()
 *-----------------------------------------------------------------------------*/
static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
			     uint32_t src, void *priv)
{
	matrix matrix_array[NUM_MATRIX];
	matrix matrix_result;

	(void)priv;
	(void)src;

	if ((*(unsigned int *)data) == SHUTDOWN_MSG) {
		LPRINTF("shutdown message is received.\r\n");
		shutdown_req = 1;
		return RPMSG_SUCCESS;
	}

	memcpy(matrix_array, data, len);
	/* Process received data and multiple matrices. */
	Matrix_Multiply(&matrix_array[0], &matrix_array[1], &matrix_result);

	/* Send the result of matrix multiplication back to host. */
	if (rpmsg_send(ept, &matrix_result, sizeof(matrix)) < 0) {
		LPERROR("rpmsg_send failed\r\n");
	}
	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	(void)ept;
	LPERROR("Endpoint is destroyed\r\n");
	shutdown_req = 1;
}

/*-----------------------------------------------------------------------------*
 *  Application
 *-----------------------------------------------------------------------------*/
// changes 

const char* rpmsg_error_string(int err) {
    switch (err) {
        case 0: return "RPMSG_SUCCESS";
        case -2001: return "RPMSG_ERR_NO_MEM";
        case -2002: return "RPMSG_ERR_NO_BUFF";
        case -2003: return "RPMSG_ERR_PARAM";
        case -2004: return "RPMSG_ERR_DEV_STATE";
        case -2005: return "RPMSG_ERR_BUFF_SIZE";
        case -2006: return "RPMSG_ERR_INIT";
        case -2007: return "RPMSG_ERR_ADDR";
        case -2008: return "RPMSG_ERR_PERM";
        default: return "Unknown error";
    }
}

int app(struct rpmsg_device *rdev, void *priv)
{
	int ret;

	ret = rpmsg_create_ept(&lept, rdev, RPMSG_SERVICE_NAME,
			       RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
			       rpmsg_endpoint_cb,
			       rpmsg_service_unbind);
	
	if (ret) 
	{

    LPERROR("Failed to create endpoint. Error code: %d (%s)\r\n", ret, rpmsg_error_string(ret));
    LPERROR("values of lept are: name=%s, addr=%lu, dest_addr=%lu\r\n",
           lept.name, lept.addr, lept.dest_addr);
    LPERROR("the values of rdev are: endpoints=%p, ns_ept.name=%s, "
           "ns_ept.addr=%lu, ns_ept.dest_addr=%lu\r\n",
           rdev->endpoints.next, rdev->ns_ept.name,
           rdev->ns_ept.addr, rdev->ns_ept.dest_addr);
    LPERROR("the value of callback is %p\r\n",
           rpmsg_endpoint_cb);
    return -1;
	}

	LPRINTF("Waiting for events...\r\n");
	while(1) {
		platform_poll(priv);
	
		/* we got a shutdown request, exit */
		if (shutdown_req) {
			LPRINTF("Shutdown request received, exiting...\r\n");
			break;
		}
	}
	rpmsg_destroy_ept(&lept);

	return 0;
}

/*-----------------------------------------------------------------------------*
 *  Application entry point
 *-----------------------------------------------------------------------------*/
void freertos_main(void *args)
{
	int argc = 0;
    char **argv = NULL;

	void *platform;
	struct rpmsg_device *rpdev;
	int ret;

	LPRINTF("Starting application...in freertos_main\r\n");

	/* Initialize platform */
	ret = platform_init(argc, argv, &platform); 
	if (ret) {
		LPERROR("Failed to initialize platform.\r\n");
		ret = -1;
	} else {
		rpdev = platform_create_rpmsg_vdev(platform, 0,
						   VIRTIO_DEV_DEVICE,
						   NULL, NULL);
		if (!rpdev) {
			LPERROR("Failed to create rpmsg virtio device.\r\n");
			ret = -1;
		} else {
			app(rpdev, platform);
			platform_release_rpmsg_vdev(rpdev, platform);
			ret = 0;
		}
	}

	
	

	LPRINTF("Stopping application...\r\n");

	platform_cleanup(platform);
	vTaskDelete(NULL);
}

int main()
{
	HwiP_init();          // --------------------  initialising the Hwi module for interrupt handling
	ClockP_init();          // --------------------  initialising the clock module for timer interrupts and context switches 
	CycleCounterP_init(SOC_getSelfCpuClk());

	
	
	HwiP_enable()	;

	/* Start the application */
	LPRINTF("Starting application...\r\n");
    /* This task is created at highest priority, it should create more tasks and then delete itself */

    gMainTask = xTaskCreateStatic( freertos_main,   /* Pointer to the function that implements the task. */
                                  "freertos_main_mera", /* Text name for the task.  This is to facilitate debugging only. */
                                  MAIN_TASK_SIZE,  /* Stack depth in units of StackType_t typically uint32_t on 32b CPUs */
                                  NULL,            /* We are not using the task parameter. */
                                  MAIN_TASK_PRI,   /* task priority, 0 is lowest priority, configMAX_PRIORITIES-1 is highest */
                                  gMainTaskStack,  /* pointer to stack base */
                                  &gMainTaskObj ); /* pointer to statically allocated task object memory */
    configASSERT(gMainTask != NULL);
	if(gMainTask==NULL){
		LPRINTF("gmainTask is NULL, task creation failed\r\n");
	}
	else {
		LPRINTF("gMainTask is %p\r\n",gMainTask);
	}

	
    vTaskStartScheduler();
	LPRINTF("The task scheduler returns\r\n");
    /* The following line should never be reached because vTaskStartScheduler()
    will only return if there was not enough FreeRTOS heap memory available to
    create the Idle and (if configured) Timer tasks.  Heap management, and
    techniques for trapping heap exhaustion, are described in the book text. */
    DebugP_assertNoLog(0);

    return 0;
}