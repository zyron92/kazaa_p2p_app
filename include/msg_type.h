#ifndef __MSG_TYPE_H__
#define __MSG_TYPE_H__

#define ERR -1
#define OK 0

#define CHILD_HELLO 0x00000010 //@HELLO from CHILD, when a child connects to a super node
#define SUPER_HELLO 0x00000011 //HELLO from SUPER, when a super node responses to a child connection trial
#define SUPER2SUPER_HELLO 0x00000012 //HELLO from SUPER to SUPER, when a super node	connects to another	super node
#define FILE_INFO 0x00000020 //@FILE INFO, when a child sends its file information to a	super node
#define FILE_INFO_OK 0x00000021 //@FILE INFO RECV SUCCESS, when a super node	answers	to	FILE INFOmsg (successfully	received)
#define FILE_INFO_ERR 0x00000022 //FILE INFO RECV ERROR, when a super node answers to FILE INFOmsg (receive fail, only consider a case of not receiving all contents)
#define SEARCH 0x00000030 //@SEARCH QUERY, when a child sends a query to	a super	node
#define SEARCH_OK 0x00000031 //@SEARCH ANS SUCCESS, when a super node returns search	results successfully
#define SEARCH_ERR 0x00000032 //SEARCH ANS FAIL, when a	super node returns failure for its query(no matched	file)
#define FILE_REQ 0x00000040 //@FILE REQ, when a child asks a	file content to another	child (NO multiple files, only one file at a time)
#define FILE_REQ_OK 0x00000041 //@FILE REQ SUCCESS, when a child delivers file contents	to a child that has	requested the file
#define FILE_REQ_ERR 0x00000042 //FILE REQ FAIL, when a child returns	error to a child that has requested	a file
#define FILE_INFO_SHARE 0x00000050 //@FILE INFO SHARE, when a super node	delivers its file info to other super node
#define FILE_INFO_SHARE_OK 0x00000051 //@FILE INFO SHARE SUCCESS, when a super node successfully receives file info
#define FILE_INFO_SHARE_ERR 0x00000052 //FILE INFO SHARE ERROR,	when a super node fails	in receiving file info (receive	fail, only consider	a case of not receiving all contents)

#endif
