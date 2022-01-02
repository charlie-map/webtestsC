struct sockaddr {
	unsigned short   sa_family;     // address family, AF_xxx
	char			 sa_data[14];   // 14 bytes of protocol address
}

// sockaddr for either IPv4 or IPv6 but larger
struct sockaddr_storage {
	sa_family_t ss_family; // address family


	// all this is padding implementation specific
	char    __ss_pad1[_SS_PAD1SIZE];
	int64_t __ss_align;
	char    __ss_pad2[_SS_PAD2SIZE];
}

// internet address (a structure for IPv4 only (historical reasons I think))
struct in_addr {
	uint32_t s_addr; // 32-bit int (4 bytes)
}

// IPv4 version of sockaddr
struct sockaddr_in {
	short int 		   sin_family;  // address family, AF_INET
	unsigned short int sin_port;    // port number
	struct in_addr     sin_addr;    // internet address
	unsigned char      sin_zero[8]; // same size as struct sockaddr
}

struct in6_addr {
	unsigned char s6_addr[16]; // IPv6 address
}

// IPv6 version of sockaddr
struct sockaddr_in6 {
	u_int16_t		sin6_family;   // address family, AF_INET6
	u_int16_t		sin6_port;     // port number, Network Byte Order
	u_int16_t		sin6_flowinfo; // IPv6 flow information
	struct in6_addr sin6_addr;	   // IPv6 address
	u_int32_t		sin6_scope_id; // Scope ID
}

struct addrinfo {
	int				 ai_flags;      // AI_PASSIVE, AI_CANONNAME, etc.
	int				 ai_family;     // AF_INET, AF_INET6, AF_UNSPEC
	int				 ai_socktype;   // SOCK_STREAM, SOCK_DGRAM
	int				 ai_protocol;   // use 0 for "any"
	size_t			 ai_addrlen;    // size of ai_addr in bytes
	struct sockadder *ai_addr;      // struct sockaddr_in or _in6
	char 			 *ai_canonname; // full canonical hostname

	srtruct addrinfo *ai_next;      // linked list, next node
}