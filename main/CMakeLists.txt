set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DESP32_GATEWAY -D__error_t_defined")

set(COMPONENT_SRCDIRS "."
	"source"
	"source/utils"
	"source/http"
	"source/handlers"
	"common"
	"cyclone_tcp/core"
	"cyclone_tcp/dhcp"
	"cyclone_tcp/dhcpv6"
	"cyclone_tcp/dns"
	"cyclone_tcp/dns_sd"
	"cyclone_tcp/drivers/mac"
	"cyclone_tcp/drivers/phy"
	"cyclone_tcp/drivers/wifi"
	"cyclone_tcp/http"
	"cyclone_tcp/igmp"
	"cyclone_tcp/ipv4"
	"cyclone_tcp/ipv6"
	"cyclone_tcp/llmnr"
	"cyclone_tcp/mdns"
	"cyclone_tcp/mibs"
	"cyclone_tcp/mqtt"
	"cyclone_tcp/netbios"
	"cyclone_tcp/ppp"
	"cyclone_tcp/web_socket"
)

set(COMPONENT_ADD_INCLUDEDIRS "."
	"common"
	"cyclone_tcp"
)

register_component()
