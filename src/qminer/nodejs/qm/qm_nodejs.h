#ifndef QMINER_QM_NODEJS
#define QMINER_QM_NODEJS

#define BUILDING_NODE_EXTENSION

#include <node.h>
#include <node_object_wrap.h>
#include "qminer.h"

// NOTE: This is *not* the same as in QMiner JS. 
#define JsDeclareProperty(Function) \
	static void Function(v8::Local<v8::String> Name, const v8::PropertyCallbackInfo<v8::Value>& Info); \
	static void _ ## Function(v8::Local<v8::String> Name, const v8::PropertyCallbackInfo<v8::Value>& Info) { \
	   v8::Isolate* Isolate = v8::Isolate::GetCurrent(); \
	   v8::HandleScope HandleScope(Isolate); \
	   try { \
	      Function(Name, Info); \
	   } catch (const PExcept& Except) { \
	      /* if(typeid(Except) == typeid(TQmExcept::New(""))) { */ \
            Isolate->ThrowException(v8::Exception::TypeError( \
               v8::String::NewFromUtf8(Isolate, "[la addon] Exception"))); \
         /* } else { \
            throw Except; \
         } */ \
	   } \
	}

#define JsDeclareFunction(Function) \
   static void Function(const v8::FunctionCallbackInfo<v8::Value>& Args); \
   static void _ ## Function(const v8::FunctionCallbackInfo<v8::Value>& Args) { \
      v8::Isolate* Isolate = v8::Isolate::GetCurrent(); \
      v8::HandleScope HandleScope(Isolate); \
      try { \
         Function(Args); \
      } catch (const PExcept& Except) { \
         /* if(typeid(Except) == typeid(TQmExcept::New(""))) { */ \
            Isolate->ThrowException(v8::Exception::TypeError( \
               v8::String::NewFromUtf8(Isolate, "[la addon] Exception"))); \
         /* } else { \
            throw Except; \
         } */ \
      } \
   }

// XXX: The macro expects that variables Args and Isolate exist. 
#define QmAssert(Cond) \
   if (!(Cond)) { \
      Args.GetReturnValue().Set(Isolate->ThrowException(v8::Exception::TypeError( \
         v8::String::NewFromUtf8(Isolate, "[la addon] Exception")))); return; }

// XXX: The macro expects that variable Args and Isolate exist. 
#define QmAssertR(Cond, MsgStr) \
  if (!(Cond)) { \
   Args.GetReturnValue().Set(Isolate->ThrowException(v8::Exception::TypeError( \
         v8::String::NewFromUtf8(Isolate, MsgStr)))); return; }

// XXX: The macro expects that variables Args and Isolate exist. 
#define QmFailR(MsgStr) \
   Args.GetReturnValue().Set(Isolate->ThrowException(v8::Exception::TypeError( \
         v8::String::NewFromUtf8(Isolate, MsgStr)))); return;

class TNodeJsUtil {
public:
   /// Extract argument ArgN property as bool
	static bool GetArgBool(const v8::FunctionCallbackInfo<v8::Value>& Args, const int& ArgN, const TStr& Property, const bool& DefVal) {
		v8::Isolate* Isolate = v8::Isolate::GetCurrent();
		v8::HandleScope HandleScope(Isolate);
		if (Args.Length() > ArgN) {
			if (Args[ArgN]->IsObject() && Args[ArgN]->ToObject()->Has(v8::String::NewFromUtf8(Isolate, Property.CStr()))) {
				v8::Handle<v8::Value> Val = Args[ArgN]->ToObject()->Get(v8::String::NewFromUtf8(Isolate, Property.CStr()));
				 EAssertR(Val->IsBoolean(),
				   TStr::Fmt("Argument %d, property %s expected to be boolean", ArgN, Property.CStr()).CStr());
				 return static_cast<bool>(Val->BooleanValue());
			}
		}
		return DefVal;
	}
   static int GetArgInt32(const v8::FunctionCallbackInfo<v8::Value>& Args, const int& ArgN, const TStr& Property, const int& DefVal) {
		v8::Isolate* Isolate = v8::Isolate::GetCurrent();
		v8::HandleScope HandleScope(Isolate);
		if (Args.Length() > ArgN) {			
			if (Args[ArgN]->IsObject() && Args[ArgN]->ToObject()->Has(v8::String::NewFromUtf8(Isolate, Property.CStr()))) {
				v8::Handle<v8::Value> Val = Args[ArgN]->ToObject()->Get(v8::String::NewFromUtf8(Isolate, Property.CStr()));
				 EAssertR(Val->IsInt32(),
				   TStr::Fmt("Argument %d, property %s expected to be int32", ArgN, Property.CStr()).CStr());
				 return Val->ToNumber()->Int32Value();
			}
		}
		return DefVal;
	}
	// gets the class name of the underlying glib object. the name is stored in an hidden variable "class"
	static TStr GetClass(const v8::Handle<v8::Object> Obj) {
		v8::Isolate* Isolate = v8::Isolate::GetCurrent();
		v8::HandleScope HandleScope(Isolate);
		v8::Local<v8::Value> ClassNm = Obj->GetHiddenValue(v8::String::NewFromUtf8(Isolate, "class"));
		const bool EmptyP = ClassNm.IsEmpty();
		if (EmptyP) { return ""; }
		v8::String::Utf8Value Utf8(ClassNm);
		return TStr(*Utf8);		
	}

	// checks if the class name of the underlying glib object matches the given string. the name is stored in an hidden variable "class"
	static bool IsClass(const v8::Handle<v8::Object> Obj, const TStr& ClassNm) {
		TStr ObjClassStr = GetClass(Obj);		
		return ObjClassStr == ClassNm;
	}
	/// Check if argument ArgN belongs to a given class
	static bool IsArgClass(const v8::FunctionCallbackInfo<v8::Value>& Args, const int& ArgN, const TStr& ClassNm) {
		v8::Isolate* Isolate = v8::Isolate::GetCurrent();
		v8::HandleScope HandleScope(Isolate);
		EAssertR(Args.Length() > ArgN, TStr::Fmt("Missing argument %d of class %s", ArgN, ClassNm.CStr()).CStr());
      EAssertR(Args[ArgN]->IsObject(), TStr("Argument expected to be '" + ClassNm + "' but is not even an object!").CStr());
		v8::Handle<v8::Value> Val = Args[ArgN];
	 	v8::Handle<v8::Object> Data = v8::Handle<v8::Object>::Cast(Val);
		TStr ClassStr = GetClass(Data);
		return ClassStr.EqI(ClassNm);
	}
};

///////////////////////////////
// NodeJs-Qminer-Base
// 
class TNodeJsBase : public node::ObjectWrap {
private:
   static TWPt<TBase> Base;
public:
   static void Init(v8::Handle<v8::Object> exports);
public:
	//# 
	//# **Functions and properties:**
	//# 
    //#- `store = qm.store(storeName)` -- store with name `storeName`; `store = null` when no such store
	JsDeclareFunction(store);
    //#- `strArr = qm.getStoreList()` -- an array of strings listing all existing stores
	JsDeclareFunction(getStoreList);
    //#- `qm.createStore(storeDef)` -- create new store(s) based on given `storeDef` (Json) [definition](Store Definition)
    //#- `qm.createStore(storeDef, storeSizeInMB)` -- create new store(s) based on given `storeDef` (Json) [definition](Store Definition)
	JsDeclareFunction(createStore);
    //#- `rs = qm.search(query)` -- execute `query` (Json) specified in [QMiner Query Language](Query Language) 
    //#   and returns a record set `rs` with results
	JsDeclareFunction(search);   
    //#- `qm.gc()` -- start garbage collection to remove records outside time windows
	JsDeclareFunction(gc);
	//#- `sa = qm.newStreamAggr(paramJSON)` -- create a new [Stream Aggregate](Stream-Aggregates) object `sa`. The constructor parameters are stored in `paramJSON` object. `paramJSON` must contain field `type` which defines the type of the aggregate.
	//#- `sa = qm.newStreamAggr(paramJSON, storeName)` -- create a new [Stream Aggregate](Stream-Aggregates) object `sa`. The constructor parameters are stored in `paramJSON` object. `paramJSON` must contain field `type` which defines the type of the aggregate. Second parameter `storeName` is used to register the stream aggregate for events on the appropriate store.
	//#- `sa = qm.newStreamAggr(paramJSON, storeNameArr)` -- create a new [Stream Aggregate](Stream-Aggregates) object `sa`. The constructor parameters are stored in `paramJSON` object. `paramJSON` must contain field `type` which defines the type of the aggregate. Second parameter `storeNameArr` is an array of store names, where the stream aggregate will be registered.
	//#- `sa = qm.newStreamAggr(funObj)` -- create a new [Stream Aggregate](Stream-Aggregates). The function object `funObj` defines the aggregate name and four callbacks: onAdd (takes record as input), onUpdate (takes record as input), onDelete (takes record as input) and saveJson (takes one numeric parameter - limit) callbacks. An example: `funObj = new function () {this.name = 'aggr1'; this.onAdd = function (rec) { }; this.onUpdate = function (rec) { }; this.onDelete = function (rec) { };  this.saveJson = function (limit) { return {}; } }`.
	//#- `sa = qm.newStreamAggr(funObj, storeName)` -- create a new [Stream Aggregate](Stream-Aggregates). The function object `funObj` defines the aggregate name and four callbacks: onAdd (takes record as input), onUpdate (takes record as input), onDelete (takes record as input) and saveJson (takes one numeric parameter - limit) callbacks. An example: `funObj = new function () {this.name = 'aggr1'; this.onAdd = function (rec) { }; this.onUpdate = function (rec) { }; this.onDelete = function (rec) { };  this.saveJson = function (limit) { return {}; } }`.  Second parameter `storeName` is used to register the stream aggregate for events on the appropriate store.
	//#- `sa = qm.newStreamAggr(funObj, storeNameArr)` -- create a new [Stream Aggregate](Stream-Aggregates). The function object `funObj` defines the aggregate name and four callbacks: onAdd (takes record as input), onUpdate (takes record as input), onDelete (takes record as input) and saveJson (takes one numeric parameter - limit) callbacks. An example: `funObj = new function () {this.name = 'aggr1'; this.onAdd = function (rec) { }; this.onUpdate = function (rec) { }; this.onDelete = function (rec) { };  this.saveJson = function (limit) { return {}; } }`.  Second parameter `storeNameArr` is an array of store names, where the stream aggregate will be registered.
	//#- `sa = qm.newStreamAggr(ftrExtObj)` -- create a new [Stream Aggregate](Stream-Aggregates). The `ftrExtObj = {type : 'ftrext', name : 'aggr1', featureSpace: fsp }` object has three parameters: `type='ftrext'`,`name` (string) and feature space `featureSpace` whose value is a feature space object.
	//#- `sa = qm.newStreamAggr(ftrExtObj, storeName)` -- create a new [Stream Aggregate](Stream-Aggregates). The `ftrExtObj = {type : 'ftrext', name : 'aggr1', featureSpace: fsp }` object has three parameters: `type='ftrext'`,`name` (string) and feature space `featureSpace` whose value is a feature space object.  Second parameter `storeName` is used to register the stream aggregate for events on the appropriate store.
	//#- `sa = qm.newStreamAggr(ftrExtObj, storeNameArr)` -- create a new [Stream Aggregate](Stream-Aggregates). The `ftrExtObj = {type : 'ftrext', name : 'aggr1', featureSpace: fsp }` object has three parameters: `type='ftrext'`,`name` (string) and feature space `featureSpace` whose value is a feature space object.  Second parameter `storeNameArr` is an array of store names, where the stream aggregate will be registered.
	JsDeclareFunction(newStreamAggr);
	//#- `sa = qm.getStreamAggr(saName)` -- gets the stream aggregate `sa` given name (string).
	JsDeclareFunction(getStreamAggr);
	//#- `strArr = qm.getStreamAggrNames()` -- gets the stream aggregate names of stream aggregates in the default stream aggregate base.
	JsDeclareFunction(getStreamAggrNames);	
	//#JSIMPLEMENT:src/qminer/qminer.js    
};

#endif