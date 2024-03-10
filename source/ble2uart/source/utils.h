/*
 * utils.h
 *
 *  Created on: 6 мар. 2024 г.
 *      Author: pvvx
 */

#ifndef _UTILS_H_
#define _UTILS_H_

inline void my_fifo_next (my_fifo_t *f)
{
	f->wptr++;
}

inline void my_fifo_pop (my_fifo_t *f)
{
	f->rptr++;
}

#endif /* _UTILS_H_ */
