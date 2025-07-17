#include <data_plane.h>

data_plane::data_plane(control_plane &control_plane) : _control_plane(control_plane) {}

void data_plane::handle_uplink(uint32_t dp_teid, Packet &&packet) {
    const auto eps_bearer = _control_plane.find_bearer_by_dp_teid(dp_teid);
    if (eps_bearer == nullptr) {
        return;
    }
    const auto pdn_con = eps_bearer->get_pdn_connection();
    if (pdn_con == nullptr) {
        return;
    }
    forward_packet_to_apn(pdn_con->get_apn_gw(), std::move(packet));
}

void data_plane::handle_downlink(const boost::asio::ip::address_v4 &ue_ip, Packet &&packet) {
    const auto pdn = _control_plane.find_pdn_by_ip_address(ue_ip);
    if (pdn == nullptr) {
        return;
    }
    const auto serv_con = pdn->get_default_bearer();
    if (serv_con == nullptr) {
        return;
    }
    forward_packet_to_sgw(pdn->get_sgw_address(),serv_con->get_sgw_dp_teid(), std::move(packet));
}



