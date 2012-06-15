// output_dal


/*
 *      Copyright (C) 2012 by Kristoff Bonne, ON1ARF
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; version 2 of the License.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */


// Release information
// version 20120322 initial release




// Receiver output destination Source Abstraction Layer

// This code is designed to be able to write to a file, stdout, TCP-stream 
// or UDP-stream using the same "output_" code.

// It abstracts the output I/O from the "output_dstar" and "output_raw"

/*
 *      Copyright (C) 2011 by Kristoff Bonne, ON1ARF
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; version 2 of the License.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */

// Release information:
// 20120224: Initial release


// COMMANDS
#define IDAL_CMD_INIT 0
#define IDAL_CMD_OPEN 1
#define IDAL_CMD_WRITE 2
#define IDAL_CMD_CLOSE 3

// return values
#define IDAL_RET_SUCCESS 0
#define IDAL_RET_FAIL 1
#define IDAL_RET_WARN 2

// can output be reopened after close?
#define IDAL_OUTPUT_REOPENYES 1
#define IDAL_OUTPUT_REOPENNO 0 // not used. All output destinations can be reopened

// error messages
#define IDAL_ERR_ALREADYINIT 0
#define IDAL_ERR_NOTINIT 1
#define IDAL_ERR_NOTOPEN 2
#define IDAL_ERR_CMDFAIL 3
#define IDAL_ERR_NOTYETIMPLEMENTED 10


// This version can only process one output stream at a time
int output_dal (int cmd, void * data, int len, int * retval2, char * retmsg) {

// vars for FILE in
static FILE * fileout;

// vars for UDP out
static int udpsocket=0;
static struct sockaddr_in6 * udpsockaddr;

static c_globaldatastr * p_c_global; 
static r_globaldatastr * p_r_global;
static g_globaldatastr * p_g_global;


// other vars
static int init=0;
static int methode=0;
static int has_been_closed=0; // 0: not been closed before, 1: has been closed and can be reopened
										// 2: has been clossed and may not be reopened
static int filecount=0;

// command INIT
// Input data = pointer to combined global data structure
// returns 0 

if (cmd == IDAL_CMD_INIT) {

	if (init) {
		snprintf(retmsg,ISALRETMSGSIZE,"Error: DAL already initialised\n");
		*retval2=IDAL_ERR_ALREADYINIT;
		return(IDAL_RET_FAIL);
	}; // end

	init=1;
	fileout=NULL;
	has_been_closed=0;
	udpsocket=-1;
	filecount=0;

	p_c_global = (c_globaldatastr *) data;
	p_r_global = p_c_global->p_r_global;
	p_g_global = p_c_global->p_g_global;

	return(IDAL_RET_SUCCESS);
}; // end if


// command OPEN
// No input data (all info taken from global data)
// return: 0 on success, 1 on failure
// retval2:
// (if success): 0 if output is ending (e.g. single file)
//               1 if output is endless (stdout, tcp)¦
// (if failure): error id

if (cmd == IDAL_CMD_OPEN) {

	if (!(init)) {
	// not initialised
		snprintf(retmsg,ISALRETMSGSIZE,"Error: DAL not initialised\n");
		*retval2=IDAL_ERR_NOTINIT;
		return(IDAL_RET_FAIL);
	}; // end

	// check "has been closed" flag
	// if 2 ("has been closed and may not be reopenen"), return
	if (has_been_closed == 2) {
		snprintf(retmsg,ISALRETMSGSIZE,"Error: Input has been closed and may not be reopened\n");
		*retval2=IDAL_ERR_CMDFAIL;
		return(IDAL_RET_FAIL);
	}; // end if

	// output methode 1: stdout
	if (p_r_global->outtostdout) {
		fileout=stdout;
		methode=1;

		// set retval2 to "1" (endless output)
		*retval2=1;
		return(0);
	}; // end if

	if (p_r_global->fnameout) {
	// output methode 2: file
		char fnamefull[160];
		// create full filename

		// methode 2: file
		methode=2;
		if (p_r_global->recformat == 1) {
			snprintf(fnamefull,160,"%s-%04d.dvtool",p_r_global->fnameout,filecount);
		} else if (p_r_global->recformat == 2) {
			snprintf(fnamefull,160,"%s-%04d.dstrudp",p_r_global->fnameout,filecount);
		} else if (p_r_global->recformat == 10) {
			snprintf(fnamefull,160,"%s-%04d.gmskraw",p_r_global->fnameout,filecount);
		} else if (p_r_global->recformat == 20) {
			snprintf(fnamefull,160,"%s-%04d.c2",p_r_global->fnameout,filecount);
		} else {
			snprintf(fnamefull,160,"%s-%04d.bin",p_r_global->fnameout,filecount);
		}; // end elsif - elsif - elsif - if
		fileout=fopen(fnamefull,"w");

		if (!(fileout)) {
			snprintf(retmsg,ISALRETMSGSIZE,"Error: could not open file %s for output\n",fnamefull);
			*retval2=IDAL_ERR_CMDFAIL;
			return(IDAL_RET_FAIL);
		};
		// success
		*retval2=0; // output is ending, not endless
		methode=2;
		return(IDAL_RET_SUCCESS);
	}; // end if

	// output methode 3: TCP
	// not yet implemented
//	if (p_r_global->tcpport) {
//		// not yet implemented
//		*retval2=IDAL_ERR_NOTYETIMPLEMENTED;
//		return(IDAL_RET_FAIL);
//	}; // end if

	// output methode 4: UDP
	if (p_r_global->udpout_port) {
		int ret;
		struct addrinfo hint;
		struct addrinfo *info;


		if ((p_r_global->udpout_port < 0) || (p_r_global->udpout_port > 65535)) {
			snprintf(retmsg,ISALRETMSGSIZE,"Error: UDP port should be between 1 and 65535\n");
			*retval2=IDAL_ERR_CMDFAIL;
			return(IDAL_RET_FAIL);
		}; // end if

		if ((p_r_global->ipv4only) && (p_r_global->ipv6only)) {
			snprintf(retmsg,ISALRETMSGSIZE,"Error: ipv4-only and ipv6-only are mutualy exclusive\n");
			*retval2=IDAL_ERR_CMDFAIL;
			return(IDAL_RET_FAIL);
		}; // end if

		// clear hint
		memset(&hint,0,sizeof(struct addrinfo));

		// init hint
		// init "hint"
		hint.ai_socktype = SOCK_DGRAM;

		// resolve hostname, use function "getaddrinfo"
		// set address family in hint if ipv4only or ipv6ony
		if (r_global.ipv4only) {
			hint.ai_family = AF_INET;
		} else if (r_global.ipv6only) {
			hint.ai_family = AF_INET6;
		} else {
			hint.ai_family = AF_UNSPEC;
		}; // end if

		// do DNS-query, use getaddrinfo for both ipv4 and ipv6 support
		ret=getaddrinfo(r_global.udpout_host, NULL, &hint, &info);

		if (ret != 0) {
			snprintf(retmsg,ISALRETMSGSIZE,"Error: resolving hostname: (%s)\n",gai_strerror(ret));
			*retval2=IDAL_ERR_CMDFAIL;
			return(IDAL_RET_FAIL);
		}; // end if

		// getaddrinfo can return multiple results, we only use the first one

		// give warning is more then one result found.
		// Data is returned in info as a linked list
		// If the "next" pointer is not NULL, there is more then one
		// element in the chain

		if ((info->ai_next != NULL) || g_global.verboselevel >= 1) {
			char ipaddrtxt[INET6_ADDRSTRLEN];


			// get ip-address in numeric form
			if (info->ai_family == AF_INET) {
				// ipv4
				// for some reason, we neem to shift the address-information with 2 positions
				// to get the correct string returned
				inet_ntop(AF_INET,&info->ai_addr->sa_data[2],ipaddrtxt,INET6_ADDRSTRLEN);
			} else {
				// ipv6
				// for some reason, we neem to shift the address-information with 6 positions
				// to get the correct string returned
				inet_ntop(AF_INET6,&info->ai_addr->sa_data[6],ipaddrtxt,INET6_ADDRSTRLEN);
			}; // end else - if

			if (p_g_global->verboselevel >= 1) {
				// store warning text tempory in tempory buffer. If no other errors pop up, we will print out this warning
				if (info->ai_next != NULL) {
					fprintf(stderr,"Warning. getaddrinfo returned multiple entries. Using %s\n",ipaddrtxt);
				} else {
					fprintf(stderr,"Sending DV-stream to ip-address %s\n",ipaddrtxt);
				}; // end if
			}; // end if
		}; // end if

		// store returned DNS info in socket address
		udpsockaddr=(struct sockaddr_in6 *) info->ai_addr;

		// fill in UDP port
		udpsockaddr->sin6_family=AF_INET6;
		udpsockaddr->sin6_port=htons((unsigned short int) r_global.udpout_port);
		udpsockaddr->sin6_scope_id=0; // scope not used here


		// create UDP socket
		udpsocket=socket(AF_INET6,SOCK_DGRAM,0);

		if (udpsocket < 0) {
			fprintf(stderr,"Error: could not create UDP socket. Shouldn't happen!\n");
			*retval2=IDAL_ERR_CMDFAIL;
			return(IDAL_RET_FAIL);
		}; // end if

		// done
		methode=4;

		*retval2=1; // ENDLESS
		return(IDAL_RET_SUCCESS);

	}; // end if

	// catchall
	// no methode found to open
	snprintf(retmsg,ISALRETMSGSIZE,"Error: unknown methode for DAL_OPEN\n");
	*retval2=IDAL_ERR_CMDFAIL;
	return(IDAL_RET_FAIL);
}; // end if



// command WRITE
// input:
// pointer to data structure
// number of octets to be writen
// return: success, failure or EOF 
// retval2:
// (if success or EOF): number of octets writen
// (if failure): error id

if (cmd == IDAL_CMD_WRITE) {
	if (!(init)) {
	// not initialised
		snprintf(retmsg,ISALRETMSGSIZE,"Error: DAL not initialised\n");
		*retval2=IDAL_ERR_NOTINIT;
		return(IDAL_RET_FAIL);
	}; // end

	if (!(methode)) {
	// not open
		snprintf(retmsg,ISALRETMSGSIZE,"Error: DAL output not opened\n");
		*retval2=IDAL_ERR_NOTOPEN;
		return(IDAL_RET_FAIL);
	}; // end


	// read from stdout or fileout
	if ((methode == 1) || (methode == 2)) {
		size_t numwrite;
		numwrite=fwrite(data,1,len,fileout);

		if ((int) numwrite < len) {
			// check for error or eof
			if (ferror(fileout)) {
				snprintf(retmsg,ISALRETMSGSIZE,"Error: write fails for file in DAL_WRITE\n");
				*retval2=IDAL_ERR_CMDFAIL;
				return(IDAL_RET_FAIL);
			}

			// no error of eof, process as success
		}; // end if

		// success
		*retval2=len;
		return(IDAL_RET_SUCCESS);
	}; // end if

	// write to udp stream
	if (methode == 4) {
		int numsend;

		numsend=sendto(udpsocket,data,len,0,(struct sockaddr*)&udpsockaddr,sizeof(udpsockaddr));

		if (numsend < 0) {
			// error 
			snprintf(retmsg,ISALRETMSGSIZE,"Error: write fails for UDP in DAL_WRITE: %d (%s)\n",errno,strerror(errno));
			*retval2=IDAL_ERR_CMDFAIL;
			return(IDAL_RET_FAIL);
		};

		// success
		*retval2=(int) len;
		return(IDAL_RET_SUCCESS);

	}; // end if

	// catchall
	// no methode found to read
	snprintf(retmsg,ISALRETMSGSIZE,"Error: unknown methode for DAL_WRITE\n");
	*retval2=IDAL_ERR_CMDFAIL;
	return(1);

}; // end command WRITE

// command CLOSE
// input: for dvtool file format, "len" contains size of file
// will be writen in header or file

// return: success, failure 
// retval2:
// (if success): REOPEN-YES or REOPEN-NO
// (if failure): error id

if (cmd == IDAL_CMD_CLOSE) {
	if (!(init)) {
	// not initialised
		snprintf(retmsg,ISALRETMSGSIZE,"Error: DAL not initialised\n");
		*retval2=IDAL_ERR_NOTINIT;
		return(IDAL_RET_FAIL);
	}; // end

	if (!(methode)) {
	// not open
		snprintf(retmsg,ISALRETMSGSIZE,"Error: DAL output not opened\n");
		*retval2=IDAL_ERR_NOTOPEN;
		return(IDAL_RET_FAIL);
	}; // end

	if (methode == 1) {
		// do not close stdout as this will cause an interruption in case of piping
		fileout=NULL;

		// reset methode 
		methode=0;


		// reopen is possible (stdout is endless stream)
		*retval2=IDAL_OUTPUT_REOPENYES;
		has_been_closed=1;

		return(IDAL_RET_SUCCESS);
	}; // end if

	if (methode == 2) {
		// close file

		// check if there is a "len" setting
		if (len) {
			int ret;
			// if yes, write length in beginning of file
			str_dvtoolheader dvtoolheader;

			memset(&dvtoolheader,0,10);
			memcpy(&dvtoolheader.headertext,"DVTOOL",6);
			dvtoolheader.filesize=(uint32_t)len;

			// rewind to beginning of file
			ret=fseek(fileout,0,SEEK_SET);

			if (!ret) {
				// write header if seek succeeded
				ret=fwrite(&dvtoolheader,10,1,fileout);
			}; // end if
			
		}; // end if


		fclose(fileout);
		fileout=NULL;

		// reset methode
		methode=0;

		// increase file counter
		filecount++;

		// reopen is not possible
		*retval2=IDAL_OUTPUT_REOPENYES;
		has_been_closed=1;

		return(IDAL_RET_SUCCESS);
	}; // end if

	if (methode == 4) {
		// close UDP socket
		close(udpsocket);
		udpsocket=-1;

		// reset methode
		methode=0;

		// reopen is possible
		*retval2=IDAL_OUTPUT_REOPENYES;
		has_been_closed=1;

		return(IDAL_RET_SUCCESS);
	}; // end if

	// catch all
	// no methode found to close
	snprintf(retmsg,ISALRETMSGSIZE,"Error: unknown methode for DAL_CLOSE\n");
	*retval2=IDAL_ERR_CMDFAIL;
	return(IDAL_RET_FAIL);

}; // end command CLOSE

// catchall

snprintf(retmsg,ISALRETMSGSIZE,"Error: unknown command %d\n",cmd);
*retval2=IDAL_ERR_CMDFAIL;
return(IDAL_RET_FAIL);
}; // end function output_sal

