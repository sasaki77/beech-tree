#pragma once

#include <cstring>
#include <functional>
#include <iostream>

#include "epics/ca/ca_context_manager.h"
#include "epics/types.h"

namespace bchtree::epics::ca {

using GetCallback = std::function<void(PVData)>;
using PutCallback = std::function<void(bool)>;
using ConnCallback = std::function<void(bool)>;
class CAPV;

template <typename T>
using GetCallbackAs = std::function<void(T)>;

template <typename T>
struct GetCBCtxAs {
    CAPV* self;
    GetCallbackAs<T> cb;
};

class CAPV {
   public:
    explicit CAPV(std::shared_ptr<CAContextManager> ctx, std::string pv_name);
    ~CAPV() noexcept;

    void AddConnCB(ConnCallback cb);
    void Connect();

    template <typename T>
    T GetAs() {
        if constexpr (std::is_same_v<T, PVData>) {
            // Don't need convert
            return pvdata_;
        } else {
            // Convert to sample data
            return extract_as<T>(pvdata_);
        }
    }

    template <typename T>
    bool GetCBAs(GetCallbackAs<T> cb, const std::chrono::milliseconds timeout) {
        auto cb_ctx = std::make_unique<GetCBCtxAs<T>>();
        cb_ctx->self = this;
        cb_ctx->cb = std::move(cb);

        // Pass cb_ctx pointer to user
        GetCBCtxAs<T>* raw = cb_ctx.release();

        const chtype dbr_type = PreferredGetType(native_type_);

        int st = ca_array_get_callback(dbr_type, static_cast<unsigned long>(1),
                                       chid_, &GetHandlerAs<T>, raw);
        if (st != ECA_NORMAL) {
            // Reclaim ownership
            std::unique_ptr<GetCBCtxAs<T>> reclaim(raw);
            std::cout << "status=" << st << " : " << ca_message(st) << "\n";
            return false;
        }
        ca_flush_io();

        return true;
    }

    bool PutCB(const PVScalarValue& v, PutCallback cb);

    std::string GetPVname() const;
    bool IsConnected() const;

   private:
    static void ConnHandler(struct connection_handler_args args);
    static void PutHandler(struct event_handler_args args);
    static void MonitorHandler(struct event_handler_args args);

    void EnsureStartMonitor(void);
    void ClearMonitor(void);

    template <typename T>
    static void GetHandlerAs(struct event_handler_args args) {
        std::unique_ptr<GetCBCtxAs<T>> cb_ctx(
            static_cast<GetCBCtxAs<T>*>(args.usr));

        if (!cb_ctx || !cb_ctx->self) return;

        if (args.status != ECA_NORMAL) {
            throw std::runtime_error(
                "get callback is called without ECA_NORMAL status");
        }
        PVData sample = DecodePVScalar(args.type, args.dbr);

        if constexpr (std::is_same_v<T, PVData>) {
            // Don't need convert
            cb_ctx->cb(sample);
        } else {
            // Convert to sample data
            cb_ctx->cb(extract_as<T>(sample));
        }
    }

    template <typename T>
    static T extract_as(const PVData& d) {
        // Try exact type first
        if (const auto* pv = std::get_if<PVScalarValue>(&d.value)) {
            if (const auto* exact = std::get_if<T>(pv)) {
                return *exact;
            }
            // Numeric scalar cast support (e.g., stored as double -> T=int32_t)
            if constexpr (std::is_same_v<T, int32_t> ||
                          std::is_same_v<T, float> ||
                          std::is_same_v<T, double> ||
                          std::is_same_v<T, uint16_t>) {
                return std::visit(
                    [](const auto& val) -> T {
                        using S = std::decay_t<decltype(val)>;
                        if constexpr (std::is_same_v<S, int32_t> ||
                                      std::is_same_v<S, float> ||
                                      std::is_same_v<S, double> ||
                                      std::is_same_v<S, uint16_t>) {
                            return static_cast<T>(val);
                        } else {
                            throw std::runtime_error("unsupported DBR type");
                        }
                    },
                    *pv);
            }
            if constexpr (std::is_same_v<T, std::string>) {
                if (const auto* s = std::get_if<std::string>(pv)) return *s;
            }
        }
        // if (const auto* pa = std::get_if<ArrayValue>(&d.value)) {
        //     if (const auto* exact = std::get_if<T>(pa)) {
        //         return *exact;
        //     }
        //     // Optionally add numeric array cast if needed
        // }
        throw std::runtime_error("unsupported DBR type");
    }

    // ---- decode helpers (TIME_ only for brevity) ----
    static PVData DecodePVScalar(chtype type, const void* dbr);
    static chtype PreferredGetType(chtype dbf);

    std::string pv_name_;
    chid chid_{nullptr};
    evid evid_{nullptr};
    bool connected_{false};
    PVData pvdata_;

    mutable std::mutex mtx_;
    std::shared_ptr<CAContextManager> ctx_;

    std::vector<ConnCallback> conn_cbs_;

    chtype native_type_ = 0;
    size_t elem_count_ = 0;
};

}  // namespace bchtree::epics::ca
