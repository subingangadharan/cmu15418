// Copyright 2013 15418 Course Staff.

#ifndef __ASST4INCLUDE_MASTER_H__
#define __ASST4INCLUDE_MASTER_H__



class Response_msg;
class Request_msg;

typedef void* Client_handle;
typedef void* Worker_handle;


/**
******************************************************************
 * Assigment 4 master node library functions (to be used by student
 * code)
 *****************************************************************
 */


/**
 * @brief Sends resp to the client designated by client_handle
 */
void send_client_response(Client_handle client_handle, const Response_msg& resp);

/**
 * @brief Send work to the worker listening on worker_handle.
 *
 * When the worker completes the task, it will respond with the
 * Response_msg containing a tag matching the tag in req (the tag
 * mechanism can be used to identify between responses from a worker
 * if a number are sent to it at the same time). When the response is
 * received, handle_worker_response() will be called.
 */
void send_request_to_worker(Worker_handle worker_handle, const Request_msg& req);

/**
 * @brief Request a new worker node
 *
 * This requests the launch of a new worker. When the worker is
 * initialized, it will message the master using
 * handle_new_worker_online, suppying the tag provided in req (this
 * can be used to identify between workers if a number are launched at
 * the same time).
 */
void request_new_worker_node(const Request_msg& req);

/**
 * @brief Kill a worker node.
 *
 * Note: Calling this function is the appropriate way to terminate a
 * worker.
 */
void kill_worker_node(Worker_handle worker_handle);

/**
 * @brief Tell the master process the server is ready to accept requests
 *
 * Notes: This call tells the system that the master node is ready to
 * start servicing requests.  For example, this would be called after
 * the master boots its initial number of worker nodes.
 */
void server_init_complete();



/**
 ******************************************************************
 * Assigment 4 master node event handlers (to be implemented by the
 * student)
 *****************************************************************
 */


/**
 * @brief Initializes all master node data-structures, etc.
 *
 * @param[out] tick_period call the handle_tick function with every
 * tick_period seconds.
 */
void master_node_init(int max_workers, int& tick_period);

/**
 * @brief Handle new work from a remote client.
 *
 * This work needs to be serviced, presumably by a worker.
 */
void handle_client_request(Client_handle client_handle, const Request_msg& req);

/**
 * @brief Handle a response from a worker.
 *
 * When work is sent to a worker, it is given a tag. When the response
 * is received from the worker, this function is called with the
 * appropriate tag.
 */
void handle_worker_response(Worker_handle worker_handle, const Response_msg& resp);

/**
 * @brief Handle creation of a new worker.
 *
 * When a worker is launched, it is given a tag. When the worker is
 * ready for work, this function is called with the appropriate tag.
 */
void handle_new_worker_online(Worker_handle worker_handle, int tag);

/**
 * @brief Handle a timer tick.
 *
 * This function is called periodically (at a frequency set by
 * init_handlers).
 */
void handle_tick();



#endif  // __ASST4INCLUDE_MASTER_H__
