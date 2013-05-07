
#include "gpio.h"

#include <v8.h>
#include <node.h>
#include <string.h>

#include <pi.h>

using namespace v8;
using namespace node;

#define ERROR(msg) \
  ThrowException(Exception::Error(String::New(msg)))

#define TYPE_ERROR(msg) \
  ThrowException(Exception::TypeError(String::New(msg)))

Persistent<FunctionTemplate> GPIO::constructor;

void
GPIO::Initialize(Handle<Object> target) {
  HandleScope scope;

  // Constructor
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  constructor = Persistent<FunctionTemplate>::New(tpl);
  constructor->InstanceTemplate()->SetInternalFieldCount(1);
  constructor->SetClassName(String::NewSymbol("GPIO"));

  // Prototype (Methods)
  SetPrototypeMethod(constructor, "setup", Setup);
  SetPrototypeMethod(constructor, "teardown", Teardown);
  SetPrototypeMethod(constructor, "stat", PinStat);
  SetPrototypeMethod(constructor, "claim", ClaimPin);
  SetPrototypeMethod(constructor, "release", ReleasePin);
  SetPrototypeMethod(constructor, "setDirection", SetPinDirection);
  //SetPrototypeMethod(constructor, "getDirection", GetPinDirection);
  //SetPrototypeMethod(constructor, "setPull", SetPinPull);
  //SetPrototypeMethod(constructor, "read", ReadPin);
  //SetPrototypeMethod(constructor, "write", WritePin);

  // Prototype (Getters/Setters)
  // Local<ObjectTemplate> proto = constructor->PrototypeTemplate();

  // Export
  target->Set(String::NewSymbol("GPIO"), constructor->GetFunction());
}

Handle<Value>
GPIO::New(const Arguments &args) {
  HandleScope scope;
  GPIO *self = new GPIO();
  self->Wrap(args.This());
  return scope.Close(args.Holder());
}

pi_gpio_direction_t
PiDirection(const Handle<String> &v8str) {
  String::AsciiValue str(v8str);
  if (!strcasecmp(*str, "in")) return PI_DIR_IN;
  if (!strcasecmp(*str, "out")) return PI_DIR_OUT;
  return PI_DIR_IN;
}

Handle<Value>
GPIO::Setup(const Arguments &args) {
  HandleScope scope;
  GPIO *self = ObjectWrap::Unwrap<GPIO>(args.Holder());

  if (!self->active) {
    int success = pi_gpio_setup();
    if (success < 0) return ERROR("gpio setup failed: are you root?");
    self->active = 1;
  }

  return scope.Close(args.Holder());
}

Handle<Value>
GPIO::Teardown(const Arguments &args) {
  HandleScope scope;
  GPIO *self = ObjectWrap::Unwrap<GPIO>(args.Holder());

  if (self->active) {
    for (int i = 0; i < PI_MAX_PINS; i++) {
      if (self->pins[i] != NULL) {
        pi_gpio_release(self->pins[i]);
        self->pins[i] = NULL;
      }
    }

    pi_gpio_teardown();
    self->active = 0;
  }

  return scope.Close(args.Holder());
}

Handle<Value>
GPIO::ClaimPin(const Arguments &args) {
  HandleScope scope;
  GPIO *self = ObjectWrap::Unwrap<GPIO>(args.Holder());

  int len = args.Length();
  if (len < 1) return TYPE_ERROR("gpio pin required");
  if (!args[0]->IsUint32()) return TYPE_ERROR("gpio pin must be a number");

  pi_gpio_pin_t gpio = args[0]->Int32Value();
  if (self->pins[gpio] != NULL) return ERROR("gpio pin already claimed");
  self->pins[gpio] = pi_gpio_claim(gpio);

  return scope.Close(args.Holder());
}

Handle<Value>
GPIO::PinStat(const Arguments &args) {
  HandleScope scope;
  GPIO *self = ObjectWrap::Unwrap<GPIO>(args.Holder());

  int len = args.Length();
  if (len < 1) return TYPE_ERROR("gpio pin required");
  if (!args[0]->IsUint32()) return TYPE_ERROR("gpio pin must be a number");

  Local<Object> res = Object::New();
  pi_gpio_pin_t gpio = args[0]->Int32Value();
  bool exist = self->pins[gpio] == NULL ? false : true;

  res->Set(String::New("claimed"), Boolean::New(exist));

  return scope.Close(res);
}

/*
char *get(v8::Local<v8::Value> value, const char *fallback = "") {
  if (value->IsString()) {
      v8::String::AsciiValue string(value);
      char *str = (char *) malloc(string.length() + 1);
      strcpy(str, *string);
      return str;
  }
  char *str = (char *) malloc(strlen(fallback) + 1);
  strcpy(str, fallback);
  return str;
}
*/

Handle<Value>
GPIO::ReleasePin(const Arguments &args) {
  HandleScope scope;
  GPIO *self = ObjectWrap::Unwrap<GPIO>(args.Holder());

  int len = args.Length();
  if (len < 1) return TYPE_ERROR("gpio pin required");
  if (!args[0]->IsUint32()) return TYPE_ERROR("gpio pin must be a number");

  pi_gpio_pin_t gpio = args[0]->Int32Value();
  if (self->pins[gpio] == NULL) return ERROR("gpio pin has not been claimed");

  pi_gpio_handle_t *handle = self->pins[gpio];
  pi_gpio_release(handle);

  // TODO: Error checking
  return scope.Close(args.Holder());
}

Handle<Value>
GPIO::SetPinDirection(const Arguments &args) {
  HandleScope scope;
  GPIO *self = ObjectWrap::Unwrap<GPIO>(args.Holder());

  int len = args.Length();
  if (len < 1) return TYPE_ERROR("gpio pin required");
  if (!args[0]->IsUint32()) return TYPE_ERROR("gpio pin must be a number");
  if (len < 2) return TYPE_ERROR("gpio direction required");
  if (!args[1]->IsString()) return TYPE_ERROR("gpio direction must be a number");

  pi_gpio_pin_t gpio = args[0]->Int32Value();
  if (!self->pins[gpio]) return ERROR("gpio pin has not been claimed");

  pi_gpio_handle_t *handle = self->pins[gpio];
  pi_gpio_direction_t direction = PiDirection(args[1]->ToString());
  pi_gpio_set_direction(handle, direction);

  // TODO: Error checking
  return scope.Close(args.Holder());
}

GPIO::GPIO () {
  active = 0;
};

GPIO::~GPIO() {};
