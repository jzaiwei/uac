#include "csenn_eXosip2.h"

#ifndef UAS_H
#define UAS_H

int invite_handle(eXosip_event_t *g_event)
{
	return 0;
}

void uas_eXosip_processEvent(void)
{
	eXosip_event_t *g_event  = NULL;/*消息事件*/
	osip_message_t *g_answer = NULL;/*请求的确认型应答*/
	pid_t pid;//thread id
	while (1)
	{
/*等待新消息的到来*/
		g_event = eXosip_event_wait(0, 200);/*侦听消息的到来*/
		eXosip_lock();
		eXosip_default_action(g_event);
		eXosip_automatic_refresh();/*Refresh REGISTER and SUBSCRIBE before the expiration delay*/
		eXosip_unlock();
		if (NULL == g_event)
		{
			continue;
		}
		csenn_eXosip_printEvent(g_event);
/*处理感兴趣的消息*/
		switch (g_event->type)
		{
/*即时消息：通信双方无需事先建立连接*/
			case EXOSIP_MESSAGE_NEW:/*MESSAGE:MESSAGE*/
			{
				printf("\r\n<EXOSIP_MESSAGE_NEW>\r\n");
				if(MSG_IS_MESSAGE(g_event->request))/*使用MESSAGE方法的请求*/
				{
					/*设备控制*/
					/*报警事件通知和分发：报警通知响应*/
					/*网络设备信息查询*/
					/*设备视音频文件检索*/
					printf("<MSG_IS_MESSAGE>\r\n");
					eXosip_lock();
					eXosip_message_build_answer(g_event->tid, 200, &g_answer);/*Build default Answer for request*/
					eXosip_message_send_answer(g_event->tid, 200, g_answer);/*按照规则回复200OK*/
					printf("eXosip_message_send_answer success!\r\n");
					eXosip_unlock();
					csenn_eXosip_paraseMsgBody(g_event);/*解析MESSAGE的XML消息体，同时保存全局会话ID*/
				}
			}
			break;
/*即时消息回复的200OK*/
			case EXOSIP_MESSAGE_ANSWERED:/*200OK*/
			{
				/*设备控制*/
				/*报警事件通知和分发：报警通知*/
				/*网络设备信息查询*/
				/*设备视音频文件检索*/
				printf("\r\n<EXOSIP_MESSAGE_ANSWERED>\r\n");
			}
			break;
/*以下类型的消息都必须事先建立连接*/
			case EXOSIP_CALL_INVITE:/*INVITE*/
			{
				printf("\r\n<EXOSIP_CALL_INVITE>\r\n");
				if(pid=fork()<0)
				{
					printf("create thread error\n");
				}
				else if(pid==0)
				{
					//child thread code
					invite_handle(g_event);
				}
				else
				{
					//father thread code
				}

				if(0&MSG_IS_INVITE(g_event->request))/*使用INVITE方法的请求*/
				{
					/*实时视音频点播*/
					/*历史视音频回放*/
					/*视音频文件下载*/
					osip_message_t *asw_msg = NULL;/*请求的确认型应答*/
					char sdp_body[4096];

					memset(sdp_body, 0, 4096);
					printf("<MSG_IS_INVITE>\r\n");

					eXosip_lock();
					if(0 != eXosip_call_build_answer(g_event->tid, 200, &asw_msg))/*Build default Answer for request*/
					{
						eXosip_call_send_answer(g_event->tid, 603, NULL);
						eXosip_unlock();
						printf("eXosip_call_build_answer error!\r\n");
						break;
					}
					eXosip_unlock();
					snprintf(sdp_body, 4096, "v=0\r\n"/*协议版本*/
											 "o=%s 0 0 IN IP4 %s\r\n"/*会话源*//*用户名/会话ID/版本/网络类型/地址类型/地址*/
											 "s=Embedded IPC\r\n"/*会话名*/
											 "c=IN IP4 %s\r\n"/*连接信息*//*网络类型/地址信息/多点会议的地址*/
											 "t=0 0\r\n"/*时间*//*开始时间/结束时间*/
											 "m=video %s RTP/AVP 96\r\n"/*媒体/端口/传送层协议/格式列表*/
											 "a=sendonly\r\n"/*收发模式*/
											 "a=rtpmap:96 H264/90000\r\n"/*净荷类型/编码名/时钟速率*/
											 "a=username:%s\r\n"
											 "a=password:%s\r\n"
											 "y=100000001\r\n"
											 "f=\r\n",
											 device_info.ipc_id,
											 device_info.ipc_ip,
											 device_info.ipc_ip,
											 device_info.ipc_port,
											 device_info.ipc_id,
											 device_info.ipc_pwd);
					eXosip_lock();
					osip_message_set_body(asw_msg, sdp_body, strlen(sdp_body));/*设置SDP消息体*/
					osip_message_set_content_type(asw_msg, "application/sdp");
					eXosip_call_send_answer(g_event->tid, 200, asw_msg);/*按照规则回复200OK with SDP*/
					printf("eXosip_call_send_answer success!\r\n");
					eXosip_unlock();
				}
			}
			break;
			case EXOSIP_CALL_ACK:/*ACK*/
			{
				/*实时视音频点播*/
				/*历史视音频回放*/
				/*视音频文件下载*/
				printf("\r\n<EXOSIP_CALL_ACK>\r\n");/*收到ACK才表示成功建立连接*/
				csenn_eXosip_paraseInviteBody(g_event);/*解析INVITE的SDP消息体，同时保存全局INVITE连接ID和全局会话ID*/
			}
			break;
			case EXOSIP_CALL_CLOSED:/*BEY*/
			{
				/*实时视音频点播*/
				/*历史视音频回放*/
				/*视音频文件下载*/
				printf("\r\n<EXOSIP_CALL_CLOSED>\r\n");
				if(MSG_IS_BYE(g_event->request))
				{
					printf("<MSG_IS_BYE>\r\n");
					if((0 != g_did_realPlay)&&(g_did_realPlay == g_event->did))/*实时视音频点播*/
					{
						/*关闭：实时视音频点播*/
						printf("realPlay closed success!\r\n");
						g_did_realPlay = 0;
					}
					else if((0 != g_did_backPlay)&&(g_did_backPlay == g_event->did))/*历史视音频回放*/
					{
						/*关闭：历史视音频回放*/
						printf("backPlay closed success!\r\n");
						g_did_backPlay = 0;
					}
					else if((0 != g_did_fileDown)&&(g_did_fileDown == g_event->did))/*视音频文件下载*/
					{
						/*关闭：视音频文件下载*/
						printf("fileDown closed success!\r\n");
						g_did_fileDown = 0;
					}
					if((0 != g_call_id)&&(0 == g_did_realPlay)&&(0 == g_did_backPlay)&&(0 == g_did_fileDown))/*设置全局INVITE连接ID*/
					{
						printf("call closed success!\r\n");
						g_call_id = 0;
					}
				}
			}
			break;
			case EXOSIP_CALL_MESSAGE_NEW:/*MESSAGE:INFO*/
			{
				/*历史视音频回放*/
				printf("\r\n<EXOSIP_CALL_MESSAGE_NEW>\r\n");
				if(MSG_IS_INFO(g_event->request))
				{
					osip_body_t *msg_body = NULL;

					printf("<MSG_IS_INFO>\r\n");
					osip_message_get_body(g_event->request, 0, &msg_body);
					if(NULL != msg_body)
					{
						eXosip_call_build_answer(g_event->tid, 200, &g_answer);/*Build default Answer for request*/
						eXosip_call_send_answer(g_event->tid, 200, g_answer);/*按照规则回复200OK*/
						printf("eXosip_call_send_answer success!\r\n");
						csenn_eXosip_paraseInfoBody(g_event);/*解析INFO的RTSP消息体*/
					}
				}
			}
			break;
			case EXOSIP_CALL_MESSAGE_ANSWERED:/*200OK*/
			{
				/*历史视音频回放*/
				/*文件结束时发送MESSAGE(File to end)的应答*/
				printf("\r\n<EXOSIP_CALL_MESSAGE_ANSWERED>\r\n");
			}
			break;
/*其它不感兴趣的消息*/
			default:
			{
				printf("\r\n<OTHER>\r\n");
				printf("eXosip event type:%d\n", g_event->type);
			}
			break;
		}
	}
}

#endif