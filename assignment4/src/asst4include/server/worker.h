#ifndef __ASST4INCLUDE_WORKER_H__
#define __ASST4INCLUDE_WORKER_H__

class Request_msg;
class Response_msg;

/**
 ******************************************************************
 * Harness interface available to worker implementation
 ******************************************************************
 */

/**
 * @brief sends response back to master
 *
 */
void worker_send_response(const Response_msg& resp);

/**
 * @brief: perform the work described by 'req', placing a response
 * string in 'resp'
 *
 * Notes: It can be assumed that is req is a request that can from the
 * client, resp is the correct response expected by the grading
 * harness.
 */
void execute_work(const Request_msg& req, Response_msg& resp);


/**
 ******************************************************************
 * Assigment 4 worker node event handlers (to be implemented by the
 * student)
 ******************************************************************
 */


/**
 * @brief Worker node init hook.
 *
 * Note: use the dictionary 'params' to pass data from the master node
 * to the worker node at worker boot time.
 */
void worker_node_init(const Request_msg& params);


/**
 * @brief Handle incoming request from master
 *
 * Notes: this function need not directly call 'worker_send_response'.
 * For example, the request could be enqueued and
 * 'worker_send_response' might be invoked by another thread.
 */
void worker_handle_request(const Request_msg& req);


#endif   // __ASST4INCLUDE_WORKER_H__
