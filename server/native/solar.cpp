#include <node.h>
#include <iostream>
#include "WSsrv.hpp"

using namespace v8;

WSsrv solar;

// Utility per convertire velocemente argomento stringa
std::string ToStdString(Isolate* isolate, Local<Value> value) {
    v8::String::Utf8Value utf8(isolate, value);
    return *utf8 ? *utf8 : "";
}

void unregisterClient(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.Length() < 1 || !args[0]->IsInt32()) {
        isolate->ThrowException(Exception::TypeError(
            String::NewFromUtf8(isolate, "Expected (int)").ToLocalChecked()
        ));
        return;
    }

    int socket = args[0]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);

   solar.unregisterClient(socket);
    // Nessun return value necessario per unregister

}

void registerClient(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.Length() < 2 || !args[0]->IsInt32() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(
            String::NewFromUtf8(isolate, "Expected (int, string)").ToLocalChecked()
        ));
        return;
    }

    int socket = args[0]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
    std::string query = ToStdString(isolate, args[1]);

    std::string xml = solar.registerClient(socket, query);

    args.GetReturnValue().Set(
        String::NewFromUtf8(isolate, xml.c_str()).ToLocalChecked()
    );
}

void render(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.Length() < 1 || !args[0]->IsInt32()) {
        isolate->ThrowException(Exception::TypeError(
            String::NewFromUtf8(isolate, "Expected (int)").ToLocalChecked()
        ));
        return;
    }

    int socket = args[0]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
    std::string xml = solar.render(socket);

    args.GetReturnValue().Set(
        String::NewFromUtf8(isolate, xml.c_str()).ToLocalChecked()
    );
}

void handleClient(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.Length() < 2 || !args[0]->IsInt32() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(
            String::NewFromUtf8(isolate, "Expected (int, string)").ToLocalChecked()
        ));
        return;
    }

    int socket = args[0]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
    std::string buffer = ToStdString(isolate, args[1]);

    std::string xml = solar.handleClient(socket, buffer);

    args.GetReturnValue().Set(
        String::NewFromUtf8(isolate, xml.c_str()).ToLocalChecked()
    );
}

void do_sendLoop(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.Length() < 1 || !args[0]->IsInt32()) {
        isolate->ThrowException(Exception::TypeError(
            String::NewFromUtf8(isolate, "Expected (int)").ToLocalChecked()
        ));
        return;
    }

    int socket = args[0]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
    std::string xml = solar.do_sendLoop(socket);
    args.GetReturnValue().Set(
        String::NewFromUtf8(isolate, xml.c_str()).ToLocalChecked()
    );
}

void Initialize(Local<Object> exports) {
    NODE_SET_METHOD(exports, "registerClient", registerClient);
    NODE_SET_METHOD(exports, "unregisterClient", unregisterClient); 
    NODE_SET_METHOD(exports, "render", render);
    NODE_SET_METHOD(exports, "handleClient", handleClient);
    NODE_SET_METHOD(exports, "do_sendLoop", do_sendLoop);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize);
