#include <node.h>
#include <iostream>
#include <exception>
#include <functional>
#include "WSsrv.hpp"

using namespace v8;

WSsrv solar;

// Native errors must reach JavaScript; exceptions cannot escape a V8 callback.
void returnNativeString(const FunctionCallbackInfo<Value>& args,
                        const std::function<std::string()>& operation) {
    Isolate* isolate = args.GetIsolate();
    try {
        const std::string result = operation();
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
    } catch (const std::exception& error) {
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, error.what()).ToLocalChecked()));
    } catch (const std::string& error) {
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, error.c_str()).ToLocalChecked()));
    } catch (...) {
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Errore motore astronomico").ToLocalChecked()));
    }
}

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

    returnNativeString(args, [&] { return solar.registerClient(socket, query); });
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
    returnNativeString(args, [&] { return solar.render(socket); });
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

    returnNativeString(args, [&] { return solar.handleClient(socket, buffer); });
}

void computeCelestialPositions(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.Length() < 2 || !args[0]->IsInt32() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(
            String::NewFromUtf8(isolate, "Expected (int, string)").ToLocalChecked()
        ));
        return;
    }

    int socket = args[0]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
    std::string buffer = ToStdString(isolate, args[1]);

    returnNativeString(args, [&] { return solar.computeCelestialPositions(socket, buffer); });
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
    returnNativeString(args, [&] { return solar.do_sendLoop(socket); });
}

void Initialize(Local<Object> exports) {
    NODE_SET_METHOD(exports, "registerClient", registerClient);
    NODE_SET_METHOD(exports, "unregisterClient", unregisterClient); 
    NODE_SET_METHOD(exports, "render", render);
    NODE_SET_METHOD(exports, "handleClient", handleClient);
    NODE_SET_METHOD(exports, "do_sendLoop", do_sendLoop);
    NODE_SET_METHOD(exports, "computeCelestialPositions", computeCelestialPositions);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize);
