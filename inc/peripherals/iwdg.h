/*
 * iwdg.h
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#ifndef __IWDG_H__
#define __IWDG_H__

/*** IWDG macros ***/

#define IWDG_REFRESH_PERIOD_SECONDS		10

/*** IWDG functions ***/

void IWDG_init(void);
void IWDG_reload(void);

#endif /* __IWDG_H__ */
