#include "epics/ca/ca_pv.h"

namespace bchtree::epics::ca {

struct GetCBCtx {
    CAPV* self;
    GetCallback cb;
};

void CAPV::AddConnCB(ConnCallback cb) {
    std::lock_guard<std::mutex> lock(mtx_);
    conn_cbs_.push_back(std::move(cb));
}

void CAPV::Connect() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (chid_) return;

    int st = ca_create_channel(pv_name_.c_str(), &ConnHandler, this,
                               CA_PRIORITY_DEFAULT, &chid_);
    if (st != ECA_NORMAL) throw std::runtime_error("ca_create_channel failed");
}

std::string CAPV::GetPVname() const { return pv_name_; };

bool CAPV::IsConnected() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return connected_;
}

void CAPV::ConnHandler(struct connection_handler_args args) {
    auto* self = static_cast<CAPV*>(ca_puser(args.chid));
    if (!self) return;

    std::lock_guard<std::mutex> lock(self->mtx_);
    self->connected_ = (args.op == CA_OP_CONN_UP);

    if (self->connected_) {
        self->native_type_ = ca_field_type(self->chid_);
        self->elem_count_ = ca_element_count(self->chid_);
    }

    std::vector<ConnCallback> cbs;
    cbs = self->conn_cbs_;
    for (auto& cb : cbs) {
        if (cb) cb(self->connected_);
    }
}

PVData CAPV::DecodePVScalar(chtype type, const void* dbr) {
    PVData data{};
    switch (type) {
        case DBR_TIME_STRING: {
            auto v = static_cast<const dbr_time_string*>(dbr);
            data.value = bchtree::epics::PVScalarValue{v->value};
            break;
        }
        case DBR_TIME_DOUBLE: {
            auto v = static_cast<const dbr_time_double*>(dbr);
            data.value = bchtree::epics::PVScalarValue{v->value};
            break;
        }
        case DBR_TIME_FLOAT: {
            auto v = static_cast<const dbr_time_float*>(dbr);
            data.value = bchtree::epics::PVScalarValue{v->value};
            break;
        }
        case DBR_TIME_LONG: {
            auto v = static_cast<const dbr_time_long*>(dbr);
            data.value = bchtree::epics::PVScalarValue{v->value};
            break;
        }
        case DBR_TIME_INT: {
            auto v = static_cast<const dbr_time_short*>(dbr);
            data.value = bchtree::epics::PVScalarValue{v->value};
            break;
        }
        case DBR_TIME_ENUM: {
            auto v = static_cast<const dbr_time_enum*>(dbr);
            data.value = bchtree::epics::PVScalarValue{v->value};
            break;
        }
        default: {
            throw std::runtime_error("unsupported DBR type");
        }
    }
    return data;
}

chtype CAPV::PreferredGetType(chtype dbf) {
    return static_cast<chtype>(dbf_type_to_DBR_TIME(dbf));
}

}  // namespace bchtree::epics::ca
