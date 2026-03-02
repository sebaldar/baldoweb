#include <node.h>
#include <iostream>

#include "WSsrv.hpp"

using namespace v8;

WSsrv solar;

void registerClient (const FunctionCallbackInfo<Value>& args) {

	Isolate *isolate=args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

	v8::Local<v8::Integer> val = args[0].As<v8::Integer>();
	v8::Local<v8::String> q = args[1].As<v8::String>();

	int socket = val->Value(); 
	int length=q->Length();

	char * query = new char[length+1];
	q->WriteUtf8 ( isolate,	query);
	
	std::string str = query;
	std::string xml = solar.registerClient ( socket, str );

	delete query;	

	

	args.GetReturnValue().Set(String::NewFromUtf8(
		isolate, xml.c_str()).ToLocalChecked());
}


void render (const FunctionCallbackInfo<Value>& args) {

	Isolate *isolate=args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

	v8::Local<v8::Integer> val = args[0].As<v8::Integer>();

	int socket = val->Value(); 
//	std::cout << "ok" << std::endl;
	std::string xml = solar.render ( socket );
	
//	args.GetReturnValue().Set(socket);	
	args.GetReturnValue().Set(String::NewFromUtf8(
		isolate, xml.c_str()).ToLocalChecked());

}

void handleClient (const FunctionCallbackInfo<Value>& args) {

	Isolate *isolate=args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

	v8::Local<v8::Integer> val = args[0].As<v8::Integer>();
	v8::Local<v8::String> q = args[1].As<v8::String>();

	int socket = val->Value(); 
	int length=q->Length();

	char * query = new char[length+1];
	q->WriteUtf8 ( isolate,	query);
	
	std::string buffer = query;
	std::string xml = solar.handleClient ( socket, buffer );

	delete query;	


	args.GetReturnValue().Set(String::NewFromUtf8(
		isolate, xml.c_str()).ToLocalChecked());

}


void do_sendLoop (const FunctionCallbackInfo<Value>& args) {

	Isolate *isolate=args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

	v8::Local<v8::Integer> val = args[0].As<v8::Integer>();

	int socket = val->Value(); 
//	std::cout << "ok" << std::endl;
	std::string xml = solar.do_sendLoop ( socket );
	
//	args.GetReturnValue().Set(socket);	
	args.GetReturnValue().Set(String::NewFromUtf8(
		isolate, xml.c_str()).ToLocalChecked());

}

void Initialize(Local<Object> exports) {
   NODE_SET_METHOD(exports, "registerClient", registerClient);
   NODE_SET_METHOD(exports, "render", render);
   NODE_SET_METHOD(exports, "handleClient", handleClient);
   NODE_SET_METHOD(exports, "do_sendLoop", do_sendLoop);
}

NODE_MODULE(addon, Initialize);


