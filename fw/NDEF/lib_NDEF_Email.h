/**
  ******************************************************************************
  * @file    lib_NDEF_Email.h
  * @author  MMY Application Team
  * @version V1.0.0
  * @date    20-November-2013
  * @brief   This file help to manage Email NDEF file.
   ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MMY-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LIB_NDEF_EMAIL_H
#define __LIB_NDEF_EMAIL_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "lib_NDEF.h"
	 
/* "mailto:customer.service@st.com?subject=M24SR S/N 754FHFGJF46G329 WARRANTY&body=this is an auomatic warranty activation email" */	 
	 
typedef struct 
{
	char EmailAdd[64];
	char Subject[100];
	char Message[2000];
	char Information[400];
}sEmailInfo;

/* Struct defined in lib_NDEF.h */
typedef struct sRecordInfo sRecordInfo;
 	 
uint16_t NDEF_ReadEmail(sRecordInfo *pRecordStruct, sEmailInfo *pEmailStruct);
uint16_t NDEF_WriteEmail( sEmailInfo *pEmailStruct );
void NDEF_PrepareEmailMessage ( sEmailInfo *pEmailStruct, uint8_t *pNDEFMessage, uint16_t *size );

#ifdef __cplusplus
}
#endif
	 
#endif /* __LIB_NDEF_EMAIL_H */

/******************* (C) COPYRIGHT 2014 STMicroelectronics *****END OF FILE****/