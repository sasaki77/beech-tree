#include "epics/ca/ca_pv.h"

namespace bchtree::epics::ca {

struct PutCBCtx {
    CAPV* self;
    PutCallback cb;
};

struct PutScalarVisitor {
    chid cid;
    PutCBCtx* cb_ctx;
    caEventCallBackFunc* handler;

    bool operator()(int32_t v) const {
        int rc = ca_put_callback(DBR_LONG, cid, &v, handler, cb_ctx);
        return (rc == ECA_NORMAL);
    }
    bool operator()(float v) const {
        int rc = ca_put_callback(DBR_FLOAT, cid, &v, handler, cb_ctx);
        return (rc == ECA_NORMAL);
    }
    bool operator()(double v) const {
        int rc = ca_put_callback(DBR_DOUBLE, cid, &v, handler, cb_ctx);
        return (rc == ECA_NORMAL);
    }
    bool operator()(uint16_t v) const {
        int rc = ca_put_callback(DBR_ENUM, cid, &v, handler, cb_ctx);
        return (rc == ECA_NORMAL);
    }
    bool operator()(const std::string& s) const {
        char buf[MAX_STRING_SIZE] = {};
        std::strncpy(buf, s.c_str(), MAX_STRING_SIZE - 1);
        int rc = ca_put_callback(DBR_STRING, cid, buf, handler, cb_ctx);
        return (rc == ECA_NORMAL);
    }
};

CAPV::CAPV(std::shared_ptr<CAContextManager> ctx, std::string pv_name)
    : pv_name_(std::move(pv_name)), ctx_(std::move(ctx)) {}

CAPV::~CAPV() {
    ClearMonitor();
    if (chid_) {
        ca_clear_channel(chid_);
        chid_ = nullptr;
    }
}

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

bool CAPV::PutCB(const PVScalarValue& v, PutCallback cb) {
    auto cb_ctx = std::make_unique<PutCBCtx>();
    cb_ctx->self = this;
    cb_ctx->cb = std::move(cb);

    // Pass cb_ctx pointer to user
    PutCBCtx* raw = cb_ctx.release();

    PutScalarVisitor visitor{chid_, raw, &PutHandler};
    bool success = std::visit(visitor, v);

    ca_flush_io();

    if (!success) {
        // Reclaim ownership
        std::unique_ptr<PutCBCtx> reclaim(raw);
        return false;
    }

    return true;
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

    // Alway start monitor for now
    self->EnsureStartMonitor();
}

void CAPV::PutHandler(struct event_handler_args args) {
    std::unique_ptr<PutCBCtx> cb_ctx(static_cast<PutCBCtx*>(args.usr));
    if (!cb_ctx || !cb_ctx->self) return;

    bool success{args.status == ECA_NORMAL};
    cb_ctx->cb(success);
}

void CAPV::MonitorHandler(struct event_handler_args args) {
    auto* self = static_cast<CAPV*>(args.usr);
    if (!self) return;

    if (args.status != ECA_NORMAL) {
        return;
    }

    std::lock_guard<std::mutex> lock(self->mtx_);
    self->pvdata_ = DecodePVScalar(args.type, args.dbr);
}

void CAPV::EnsureStartMonitor() {
    if (!connected_ or !chid_) return;  // Not connected
    if (evid_) return;                  // Alread started

    const short dbf = ca_field_type(chid_);
    const chtype dbr_type = PreferredGetType(native_type_);

    // Only scalar value is supported now
    const unsigned long cnt = 1;

    int st = ca_create_subscription(dbr_type, cnt, chid_, DBE_VALUE | DBE_ALARM,
                                    &CAPV::MonitorHandler, this, &evid_);
    if (st != ECA_NORMAL) {
        std::cout << "status=" << st << " : " << ca_message(st) << "\n";
    }
}

void CAPV::ClearMonitor() {
    if (!evid_) return;  // Not started

    ca_clear_subscription(evid_);
    evid_ = nullptr;
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
