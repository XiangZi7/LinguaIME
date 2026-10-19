#include "text_service.h"
#include "diagnostics.h"
#include <new>

namespace {
const GUID* const kModeCompartments[] = {
    &GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
    &GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION,
};

struct PublishingScope {
    bool& active;
    explicit PublishingScope(bool& value) : active(value) {
        active = true;
    }
    ~PublishingScope() {
        active = false;
    }
};
}

HRESULT TextService::initialize_language_bar() {
    ComPtr<ITfCompartmentMgr> compartments;
    auto result = thread_manager_.As(&compartments);
    for (size_t i = 0; SUCCEEDED(result) && i < mode_compartments_.size(); ++i) {
        auto& compartment = mode_compartments_[i];
        result = compartments->GetCompartment(*kModeCompartments[i], &compartment.value);
        if (SUCCEEDED(result)) result = compartment.value.As(&compartment.source);
        if (SUCCEEDED(result)) {
            result = compartment.source->AdviseSink(
                IID_ITfCompartmentEventSink, static_cast<ITfCompartmentEventSink*>(this),
                &compartment.cookie);
        }
    }
    if (FAILED(result)) return result;
    chinese_mode_ = true;
    result = publish_input_mode(chinese_mode_);
    if (FAILED(result)) return result;
    result = thread_manager_.As(&language_bar_manager_);
    if (FAILED(result)) return result;
    language_bar_button_.Attach(new (std::nothrow) LanguageBarButton([this]() {
        // An edit can deactivate the TIP synchronously; retain it only during this callback.
        const ComPtr<TextService> keep_alive(this);
        return set_input_mode(!chinese_mode_);
    }));
    if (!language_bar_button_) return E_OUTOFMEMORY;
    return language_bar_manager_->AddItem(language_bar_button_.Get());
}

void TextService::uninitialize_language_bar() {
    if (language_bar_button_) {
        language_bar_button_->detach();
        if (language_bar_manager_)
            language_bar_manager_->RemoveItem(language_bar_button_.Get());
        language_bar_button_.Reset();
    }
    language_bar_manager_.Reset();
    for (auto& compartment : mode_compartments_) {
        if (compartment.source && compartment.cookie != TF_INVALID_COOKIE)
            compartment.source->UnadviseSink(compartment.cookie);
        compartment.cookie = TF_INVALID_COOKIE;
        compartment.source.Reset();
        compartment.value.Reset();
    }
}

HRESULT TextService::publish_input_mode(bool chinese) {
    PublishingScope scope(publishing_mode_);
    for (size_t i = 0; i < mode_compartments_.size(); ++i) {
        if (!mode_compartments_[i].value) return E_UNEXPECTED;
        const LONG desired = chinese ? (i == 0 ? 1 : TF_CONVERSIONMODE_NATIVE) : 0;
        VARIANT current{};
        const auto read = mode_compartments_[i].value->GetValue(&current);
        const bool unchanged = SUCCEEDED(read) && current.vt == VT_I4 && current.lVal == desired;
        VariantClear(&current);
        if (unchanged) continue;
        VARIANT value{};
        value.vt = VT_I4;
        value.lVal = desired;
        const auto result = mode_compartments_[i].value->SetValue(client_id_, &value);
        if (FAILED(result)) return result;
    }
    return S_OK;
}

HRESULT TextService::set_input_mode(bool chinese) {
    if (chinese_mode_ != chinese && !buffer_.empty() && context_) {
        // Finish the existing Chinese composition before switching to direct English input.
        const auto text = page_.item_count ? wide(page_.items[selected_].text)
                                          : wide(buffer_.c_str());
        const auto result = request_edit(context_.Get(), text, true, false);
        if (FAILED(result)) return result;
        clear_state();
    }
    const auto result = publish_input_mode(chinese);
    if (FAILED(result)) {
        publish_input_mode(chinese_mode_);
        return result;
    }
    chinese_mode_ = chinese;
    if (language_bar_button_) language_bar_button_->set_mode(chinese);
    return S_OK;
}

HRESULT TextService::OnChange(REFGUID id) {
    if (publishing_mode_) return S_OK;
    try {
        for (size_t i = 0; i < mode_compartments_.size(); ++i) {
            if (id != *kModeCompartments[i] || !mode_compartments_[i].value) continue;
            VARIANT value{};
            auto result = mode_compartments_[i].value->GetValue(&value);
            if (FAILED(result)) return result;
            const bool valid = value.vt == VT_I4;
            const bool chinese = valid && (i == 0 ? value.lVal != 0
                : (value.lVal & TF_CONVERSIONMODE_NATIVE) != 0);
            VariantClear(&value);
            if (!valid) return S_OK;
            result = set_input_mode(chinese);
            if (FAILED(result)) {
                publish_input_mode(chinese_mode_);
                log_error("INPUT_MODE_CHANGE_FAILED", result);
            }
            return result;
        }
        return S_OK;
    } catch (...) {
        return E_FAIL;
    }
}
