#include <control_plane.h>
#include <ranges>
#include <utility>


std::shared_ptr<pdn_connection> control_plane::find_pdn_by_cp_teid(uint32_t cp_teid) const {
    if (_pdns.contains(cp_teid)) {
        return _pdns.at(cp_teid);
    }
    return nullptr;
}


std::shared_ptr<pdn_connection> control_plane::find_pdn_by_ip_address(const boost::asio::ip::address_v4 &ip) const {
    if (_pdns_by_ue_ip_addr.contains(ip)) {
        return _pdns_by_ue_ip_addr.at(ip);
    }
    return nullptr;
}

std::shared_ptr<bearer> control_plane::find_bearer_by_dp_teid(uint32_t dp_teid) const {
    if (_bearers.contains(dp_teid)) {
        return _bearers.at(dp_teid);
    }
    return nullptr;
}

std::shared_ptr<pdn_connection> control_plane::create_pdn_connection(const std::string &apn,
                                                                     boost::asio::ip::address_v4 sgw_addr,
                                                                     uint32_t sgw_cp_teid) {

    const auto apn_it = _apns.find(apn);
    if (apn_it == _apns.end()) {
        return nullptr;
    }
    const auto apn_gw = apn_it->second;
    int32_t cp_teid = 1;
    while (_pdns.contains(cp_teid)) {
        ++cp_teid;
    }
    uint32_t host_part = 1;
    boost::asio::ip::address_v4 ue_ip;
    for (;;) {
        ue_ip = boost::asio::ip::address_v4((10u << 24) | (0u << 16) | (0u << 8) | host_part);
        if (!_pdns_by_ue_ip_addr.contains(ue_ip)) {
            break;
        }
        ++host_part;
    }
    auto pdn = pdn_connection::create(cp_teid, apn_gw, ue_ip);
    pdn->set_sgw_cp_teid(sgw_cp_teid);
    pdn->set_sgw_addr(std::move(sgw_addr));
    _pdns.insert(std::make_pair(cp_teid, pdn));
    _pdns_by_ue_ip_addr.insert(std::make_pair(ue_ip, pdn));
    return pdn;
}

void control_plane::delete_pdn_connection(uint32_t cp_teid) {
    if (_pdns.contains(cp_teid)) {
        _pdns.erase(cp_teid);
    } else
        return;
    for (const auto &key: _bearers | std::views::keys) {
        if (key == cp_teid) {
            _bearers.erase(key);
        }
    }
    if (_pdns.contains(cp_teid)) {
        _pdns.erase(cp_teid);
    }
}

std::shared_ptr<bearer> control_plane::create_bearer(const std::shared_ptr<pdn_connection> &pdn, uint32_t sgw_teid) {

    if (!pdn) {
        return nullptr;
    }
    uint32_t dp_teid = 1;
    while (_bearers.contains(dp_teid)) {
        ++dp_teid;
    }

    auto eps_bearer = std::make_shared<bearer>(dp_teid, *pdn);
    _bearers.insert(std::make_pair(dp_teid, eps_bearer));
    eps_bearer->set_sgw_dp_teid(sgw_teid);
    pdn->add_bearer(eps_bearer);

    return eps_bearer;
}

void control_plane::delete_bearer(uint32_t dp_teid) {
    if (_bearers.contains(dp_teid)) {
        _bearers.erase(dp_teid);
    } else
        return;
    const auto eps_bearer = _bearers.find(dp_teid)->second;
    if (const auto pdn = eps_bearer->get_pdn_connection()) {
        if (pdn->get_default_bearer() == eps_bearer) {
            pdn->set_default_bearer(nullptr);
        }
        pdn->remove_bearer(dp_teid);
    }
}

    void control_plane::add_apn(const std::string &apn_name, boost::asio::ip::address_v4 apn_gateway) {
        _apns[apn_name] = std::move(apn_gateway);
    }
