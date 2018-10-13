#include "component.h"

namespace kagami {
  GroupTypeEnum GetGroupType(Object &A, Object &B) {
    auto data_A = GetObjectStuff<string>(A),
      data_B = GetObjectStuff<string>(B);
    auto data_type_A = A.GetTokenType();
    auto data_type_B = B.GetTokenType();

    GroupTypeEnum group_type = GroupTypeEnum::G_NUL;
    if (data_type_A == T_FLOAT || data_type_B == T_FLOAT) group_type = G_FLOAT;
    if (data_type_A == T_INTEGER && data_type_B == T_INTEGER) group_type = G_INT;
    if (Kit::IsString(data_A) || Kit::IsString(data_B)) group_type = G_STR;
    if ((data_A == kStrTrue || data_A == kStrFalse) && (data_B == kStrTrue || data_B == kStrFalse)) group_type = G_STR;
    return group_type;
  }

  inline bool IsStringObject(Object &obj) {
    auto id = obj.GetTypeId();
    return (id == kTypeIdRawString || id == kTypeIdString);
  }

  inline bool CheckObjectType(Object &obj, string type_id) {
    return (obj.GetTypeId() == type_id);
  }

  inline bool CheckTokenType(Object &obj, TokenTypeEnum tokenType) {
    return (obj.GetTokenType() == tokenType);
  }

  inline bool IsRawStringObject(Object &obj) {
    return CheckObjectType(obj, kTypeIdRawString);
  }

  inline Message IllegalCallMsg(string str) {
    return Message(kStrFatalError, kCodeIllegalCall, str);
  }

  inline Message IllegalParmMsg(string str) {
    return Message(kStrFatalError, kCodeIllegalParm, str);
  }

  inline Message IllegalSymbolMsg(string str) {
    return Message(kStrFatalError, kCodeIllegalSymbol, str);
  }

  inline Message CheckEntryAndStart(string id, string type_id, ObjectMap &parm) {
    Message msg;
    auto ent = entry::Order(id, type_id);
    ent.Good() ?
      msg = ent.Start(parm) :
      msg.SetCode(kCodeIllegalCall);
    return msg;
  }

  string BinaryOperations(Object &A, Object &B, string OP) {
    Kit kit;
    string temp;
    using entry::OperatorCode;
    auto op_code = entry::GetOperatorCode(OP);
    auto data_A = GetObjectStuff<string>(A),
      data_B = GetObjectStuff<string>(B);
    auto group_type = GetGroupType(A, B);

    if (group_type == G_INT || group_type == G_FLOAT) {
      switch (op_code) {
      case OperatorCode::ADD:
      case OperatorCode::SUB:
      case OperatorCode::MUL:
      case OperatorCode::DIV:
        switch (group_type) {
        case G_INT:
          temp = to_string(kit.Calc(stoi(data_A), stoi(data_B), OP));
          break;
        case G_FLOAT:
          temp = to_string(kit.Calc(stod(data_A), stod(data_B), OP)); 
          break;
        default:
          break;
        }
        break;
      case OperatorCode::IS:
      case OperatorCode::MORE_OR_EQUAL:
      case OperatorCode::LESS_OR_EQUAL:
      case OperatorCode::NOT_EQUAL:
      case OperatorCode::MORE:
      case OperatorCode::LESS:
        switch (group_type) {
        case G_INT:
          Kit::MakeBoolean(kit.Logic(stoi(data_A), stoi(data_B), OP), temp);
          break;
        case G_FLOAT:
          Kit::MakeBoolean(kit.Logic(stod(data_A), stod(data_B), OP), temp);
          break;
        default:
          break;
        }
        break;
      case OperatorCode::BIT_AND:
      case OperatorCode::BIT_OR:

        break;

      default:
        break;
      }
    }
    else if (group_type == G_STR) {
      switch (op_code) {
      case OperatorCode::ADD:
        if (data_A.front() != '\'') data_A = "'" + data_A;
        if (data_A.back() == '\'') data_A = data_A.substr(0, data_A.size() - 1);
        if (data_B.front() == '\'') data_B = data_B.substr(1, data_B.size() - 1);
        if (data_B.back() != '\'') data_B = data_B + "'";
        temp = data_A + data_B;
        break;
      case OperatorCode::IS:
      case OperatorCode::NOT_EQUAL:
        Kit::MakeBoolean(kit.Logic(data_A, data_B, OP), temp);
        break;
      case OperatorCode::AND:
        if (Kit::IsBoolean(data_A) && Kit::IsBoolean(data_B)) {
          Kit::MakeBoolean((data_A == kStrTrue && data_B == kStrTrue), temp);
        }
        else {
          temp = kStrFalse;
        }
        break;
      case OperatorCode::OR:
        if(Kit::IsBoolean(data_A) && Kit::IsBoolean(data_B)) {
          Kit::MakeBoolean((data_A == kStrTrue || data_B == kStrTrue), temp);
        }
        else {
          temp = kStrFalse;
        }
        break;
      default:
        break;
      }
    }
    return temp;
  }

  inline Message MakeOperatorMsg(ObjectMap &p, string op) {
    return Message(BinaryOperations(p["first"], p["second"], op));
  }

  Message LogicEqualOperation(ObjectMap &p, bool reverse) {
    Message msg;

    if (!p.CheckTypeId("first", IsStringObject) &&
      !p.CheckTypeId("second", IsStringObject)) {

      msg = CheckEntryAndStart("__compare", p["first"].GetTypeId(), p);

      msg.GetDetail() == kStrTrue && reverse ?
        msg.SetDetail(kStrFalse) :
        msg.SetDetail(kStrTrue);
    }
    else {
      reverse ?
        msg = MakeOperatorMsg(p, "!=") :
        msg = MakeOperatorMsg(p, "==");
    }

    return msg;
  }

  inline Message MakeConditionMsg(ObjectMap &p, int code) {
    return Message(p.Get<string>("state"), code, kStrEmpty);
  }

  Message Define(ObjectMap &p) {
    vector<string> def_head;
    size_t count = 0;
    def_head.emplace_back(p.Get<string>("id"));

    for (auto &unit : p) {
      if (unit.first == "arg" + to_string(count)) {
        string str = GetObjectStuff<string>(unit.second);
        def_head.emplace_back(str);
        count++;
      }
    }

    string def_head_string = Kit::CombineStringVector(def_head);
    return Message(kStrEmpty, kCodeDefineSign, def_head_string);
  }

  Message ReturnSign(ObjectMap &p) {
    Object &value_obj= p["value"];
    string type_id = value_obj.GetTypeId();
    auto &container = entry::GetCurrentContainer();

    Object obj;

    if (type_id != kTypeIdNull) {
      obj.Set(value_obj.Get(), type_id, value_obj.GetMethods(), false)
        .SetTokenType(value_obj.GetTokenType());
      container.Add(kStrRetValue, obj);
    }

    return Message(kStrStopSign, kCodeSuccess, kStrEmpty);
  }

  Message WriteLog(ObjectMap &p) {
    Message result;
    ofstream ofs("kagami-script.log", std::ios::out | std::ios::app);

    if (p.CheckTypeId("msg",IsStringObject)) {
      string str = p.Get<string>("msg");
      if (Kit::IsString(str)) {
        ofs << str.substr(1, str.size() - 2) << "\n";
      }
      else {
        ofs << str << "\n";
      }
    }

    ofs.close();
    return result;
  }



  string IncAndDecOperation(Object &obj, bool negative, bool keep) {
    string res, origin;

    if (CheckObjectType(obj, kTypeIdRawString)) {
      origin = GetObjectStuff<string>(obj);
      if (CheckTokenType(obj, T_INTEGER)) {
        int data = stoi(origin);
        negative ?
          data -= 1 :
          data += 1;
        keep ?
          res = origin :
          res = to_string(data);
        obj.Copy(MakeObject(data));
      }
      else {
        res = origin;
      }
    }

    return res;
  }

  inline void CheckSelfOperatorMsg(Message &msg, string res) {
    res.empty() ?
      msg = Message(res) :
      msg = IllegalParmMsg("Illegal self-operator.");
  }

  Message SelfOperator(ObjectMap &p, bool negative, bool keep) {
    Object &obj = p["object"];
    //TODO:error
    string result = IncAndDecOperation(obj, negative, keep);
    return Message(result);
  }


  Message GetRawStringType(ObjectMap &p) {
    string result;
    if (p.CheckTypeId("object",kTypeIdRawString)) {
      string str = p.Get<string>("object");

      Kit::IsString(str) ? 
        str = Kit::GetRawString(str) : 
        str = str;

      switch (Kit::GetTokenType(str)) {
      case T_BOOLEAN:result = "'boolean'"; break;
      case T_GENERIC:result = "'generic'"; break;
      case T_INTEGER:result = "'integer'"; break;
      case T_FLOAT:result = "'float'"; break;
      case T_SYMBOL:result = "'symbol'"; break;
      case T_BLANK:result = "'blank'"; break;
      case T_STRING:result = "'string'"; break;
      case T_NUL:result = "'null'"; break;
      default:result = "'null'"; break;
      }
    }
    else {
      result = "'null'";
    }
    return Message(result);
  }

  Message GetTypeId(ObjectMap &p) {
    Object &obj = p["object"];
    return Message(obj.GetTypeId());
  }

  Message BindAndSet(ObjectMap &p) {
    Object &obj = p["object"], source = p["source"];
    ObjectPointer target_obj = nullptr;
    bool existed;
    Message msg;
    string obj_id;

    if (obj.IsRef()) {
      existed = true;
      target_obj = &obj;
    }
    else {
      if (obj.GetTypeId() != kTypeIdRawString) {
        msg = IllegalParmMsg("Illegal bind operation.");
        return msg;
      }
      obj_id = GetObjectStuff<string>(obj);
      target_obj = entry::FindObject(obj_id);
      existed = !(target_obj == nullptr);
    }
    
    if (existed) {
      if (target_obj->IsRo()) {
        msg = IllegalCallMsg("Object is read-only.");
      }
      else {
        auto copy = type::GetObjectCopy(source);
        target_obj->Set(copy, source.GetTypeId(), source.GetMethods(), false)
          .SetTokenType(source.GetTokenType());
      }
    }
    else {
      Object base;
      auto copy = type::GetObjectCopy(source);
      base.Set(copy, source.GetTypeId(), source.GetMethods(), false)
        .SetTokenType(source.GetTokenType());
      auto result = entry::CreateObject(obj_id, base);
      if (result == nullptr) {
        msg = IllegalCallMsg("Object creation failed.");
      }
    }
    return msg;
  }

  Message Print(ObjectMap &p) {
    Message msg;
    Object &obj = p[kStrObject];

    auto errorMsg = []() {
      std::cout << "You can't print this object." << std::endl;
    };

    if (!Kit::FindInStringGroup("__print", obj.GetMethods())) {
      errorMsg();
    } 
    else {
      Message tempMsg = CheckEntryAndStart("__print", obj.GetTypeId(), p);
      if (tempMsg.GetCode() == kCodeIllegalCall) errorMsg();
    }
    return msg;
  }

  Message TimeReport(ObjectMap &p) {
    auto now = time(nullptr);
#if defined(_WIN32) && defined(_MSC_VER)
    char nowTime[30] = { ' ' };
    ctime_s(nowTime, sizeof(nowTime), &now);
    string str(nowTime);
    str.pop_back(); //erase '\n'
    return Message("'" + str + "'");
#else
    string TimeData(ctime(&now));
    return Message("'" + TimeData + "'");
#endif
  }

  Message Input(ObjectMap &p) {
    if (p.Search("msg")) {
      ObjectMap obj_map;
      Object empty_obj;

      obj_map.Input("not_wrap", empty_obj);
      obj_map.Input(kStrObject, p["msg"]);
      Print(obj_map);
    }

    string buf;
    std::getline(std::cin, buf);
    return Message("'" + buf + "'");
  }

  Message Convert(ObjectMap &p) {
    Message msg;

    if (!p.CheckTypeId("object",kTypeIdRawString)) {
      msg = Message(kStrFatalError, kCodeBadExpression, "Cannot convert to basic type(01)");
    }
    else {
      Object objTarget;
      string origin = p.Get<string>("object");

      Kit::IsString(origin) ?
        origin = Kit::GetRawString(origin) :
        origin = origin;

      auto type = Kit::GetTokenType(origin);
      string str;
      (type == T_NUL || type == T_GENERIC) ?
        str = kStrNull :
        str = origin;
      objTarget.Manage(str, type);
      msg.SetObject(objTarget);
    }

    return msg;
  }

  Message Quit(ObjectMap &p) {
    Message result(kStrEmpty, kCodeQuit, kStrEmpty);
    return result;
  }

  Message Nop(ObjectMap &p) {
    int size = stoi(p.Get<string>("__size"));
    Object &last_obj = p["nop" + to_string(size - 1)];
    Message msg;
    msg.SetObject(last_obj);
    return msg;
  }

  Message ArrayMaker(ObjectMap &p) {
    vector<Object> base;
    Message msg;
    int size = stoi(p.Get<string>("__size"));

    if (!p.empty()) {
      for (int i = 0; i < size; i++) {
        base.emplace_back(p["item" + to_string(i)]);
      }
    }

    msg.SetObject(Object()
      .Set(make_shared<vector<Object>>(base),
        kTypeIdArrayBase,
        type::GetMethods(kTypeIdArrayBase),
        false)
      .SetConstructorFlag());

    return msg;
  }

  Message Dir(ObjectMap &p) {
    Object &obj = p["object"];
    auto vec = Kit::BuildStringVector(obj.GetMethods());
    Message msg;
    vector<Object> output;

    for (auto &unit : vec) {
      output.emplace_back(Object()
        .Set(make_shared<string>(unit), 
          kTypeIdString, 
          type::GetMethods(kTypeIdString), 
          true));
    }

    msg.SetObject(Object()
      .Set(make_shared<vector<Object>>(output),
        kTypeIdArrayBase,
        type::GetMethods(kTypeIdArrayBase),
        true)
      .SetConstructorFlag());

    return msg;
  }


  Message Exist(ObjectMap &p){
    Object &obj = p["object"];
    string target = Kit::GetRawString(p.Get<string>("id"));
    bool result = Kit::FindInStringGroup(target, obj.GetMethods());
    Message msg;
    result ?
      msg = Message(kStrTrue) :
      msg = Message(kStrFalse);
    return msg;
  }

  Message TypeAssert(ObjectMap &p) {
    Object &obj = p["object"];
    string target = p.Get<string>("id");
    bool result = Kit::FindInStringGroup(target, obj.GetMethods());
    Message msg;
    result ?
      msg = Message(kStrTrue) :
      msg = IllegalCallMsg("Method not found - " + target);
    return msg;
  }

  Message Case(ObjectMap &p) {
    Object &obj = p["object"];
    auto copy = type::GetObjectCopy(obj);
    Object base;

    if (!IsStringObject(obj)) {
      //TODO:Re-design
      return IllegalParmMsg("Case-When is not supported yet.(01)");
    }

    base.Set(copy, obj.GetTypeId(), obj.GetMethods(), false);
    entry::CreateObject("__case", base);
    return Message(kStrTrue).SetCode(kCodeCase);
  }

  Message When(ObjectMap &p) {
    //TODO:Re-Design
    int size = stoi(p.Get<string>("__size"));
    ObjectPointer case_head = entry::FindObject("__case");
    string case_content = GetObjectStuff<string>(*case_head);
    string type_id, id;
    bool result = false, state = true;

    for (int i = 0; i < size; ++i) {
      id = "value" + to_string(i);

      if (!p.Search(id)) break;
      if (!p.CheckTypeId(id,kTypeIdRawString)) {
        state = false;
        break;
      }

      string content = p.Get<string>(id);

      if (content == case_content) {
        result = true;
        break;
      }
    }

    Message msg;
    if (!state) {
      msg = IllegalParmMsg("Case-When is not supported yet.(02)");
    }
    result ? 
      msg = Message(kStrTrue, kCodeWhen, kStrEmpty) : 
      msg = Message(kStrFalse, kCodeWhen, kStrEmpty);
    return msg;
  }

  Message Plus(ObjectMap &p) { return MakeOperatorMsg(p, "+"); }
  Message Sub(ObjectMap &p) { return MakeOperatorMsg(p, "-"); }
  Message Multiply(ObjectMap &p) { return MakeOperatorMsg(p, "*"); }
  Message Divide(ObjectMap &p) { return MakeOperatorMsg(p, "/"); }
  Message Less(ObjectMap &p) { return MakeOperatorMsg(p, "<"); }
  Message More(ObjectMap &p) { return MakeOperatorMsg(p, ">"); }
  Message LessOrEqual(ObjectMap &p) { return MakeOperatorMsg(p, "<="); }
  Message MoreOrEqual(ObjectMap &p) { return MakeOperatorMsg(p, ">="); }
  Message And(ObjectMap &p) { return MakeOperatorMsg(p, "&&"); }
  Message Or(ObjectMap &p) { return MakeOperatorMsg(p, "||"); }

  Message End(ObjectMap &p) { return Message(kStrEmpty, kCodeTailSign, kStrEmpty); }
  Message Else(ObjectMap &p) { return Message(kStrTrue, kCodeConditionLeaf, kStrEmpty); }
  Message If(ObjectMap &p) { return MakeConditionMsg(p, kCodeConditionRoot); }
  Message Elif(ObjectMap &p) { return MakeConditionMsg(p, kCodeConditionBranch); }
  Message While(ObjectMap &p) { return MakeConditionMsg(p, kCodeHeadSign); }
  Message Continue(ObjectMap &p) { return Message(kStrEmpty, kCodeContinue, kStrEmpty); }
  Message Break(ObjectMap &p) { return Message(kStrEmpty, kCodeBreak, kStrEmpty); }

  Message LogicEqual(ObjectMap &p) { return LogicEqualOperation(p, false); }
  Message LogicNotEqual(ObjectMap &p) { return LogicEqualOperation(p, true); }

  Message LeftSelfIncreament(ObjectMap &p) { return SelfOperator(p, false, false); }
  Message LeftSelfDecreament(ObjectMap &p) { return SelfOperator(p, true, false); }
  Message RightSelfIncreament(ObjectMap &p) { return SelfOperator(p, false, true); }
  Message RightSelfDecreament(ObjectMap &p) { return SelfOperator(p, true, true); }

  void GenericRegister() {
    using namespace entry;
    AddGenericEntry(Entry(Nop, "nop", GT_NOP, kCodeAutoSize));
    AddGenericEntry(Entry(ArrayMaker, "item", GT_ARRAY, kCodeAutoSize));
    AddGenericEntry(Entry(End, "", GT_END));
    AddGenericEntry(Entry(Else, "", GT_ELSE));
    AddGenericEntry(Entry(If, "state", GT_IF));
    AddGenericEntry(Entry(While, "state", GT_WHILE));
    AddGenericEntry(Entry(Elif, "state", GT_ELIF));
    AddGenericEntry(Entry(BindAndSet, "object|source", GT_BIND, kCodeNormalParm, 0));
    AddGenericEntry(Entry(Plus, "first|second", GT_ADD, kCodeNormalParm, 2));
    AddGenericEntry(Entry(Sub, "first|second", GT_SUB, kCodeNormalParm, 2));
    AddGenericEntry(Entry(Multiply, "first|second", GT_MUL, kCodeNormalParm, 3));
    AddGenericEntry(Entry(Divide, "first|second", GT_DIV, kCodeNormalParm, 3));
    AddGenericEntry(Entry(LogicEqual, "first|second", GT_IS, kCodeNormalParm, 1));
    AddGenericEntry(Entry(LessOrEqual, "first|second", GT_LESS_OR_EQUAL, kCodeNormalParm, 1));
    AddGenericEntry(Entry(MoreOrEqual, "first|second", GT_MORE_OR_EQUAL, kCodeNormalParm, 1));
    AddGenericEntry(Entry(LogicNotEqual, "first|second", GT_NOT_EQUAL, kCodeNormalParm, 1));
    AddGenericEntry(Entry(More, "first|second", GT_MORE, kCodeNormalParm, 1));
    AddGenericEntry(Entry(Less, "first|second", GT_LESS, kCodeNormalParm, 1));
    AddGenericEntry(Entry(LeftSelfIncreament, "object", GT_LSELF_INC));
    AddGenericEntry(Entry(LeftSelfDecreament, "object", GT_LSELF_DEC));
    AddGenericEntry(Entry(RightSelfIncreament, "object", GT_RSELF_INC));
    AddGenericEntry(Entry(RightSelfDecreament, "object", GT_RSELF_DEC));
    AddGenericEntry(Entry(And, "first|second", GT_AND, kCodeNormalParm, 1));
    AddGenericEntry(Entry(Or, "first|second", GT_OR, kCodeNormalParm, 1));
    AddGenericEntry(Entry(Define, "id|arg", GT_DEF, kCodeAutoSize));
    AddGenericEntry(Entry(ReturnSign, "value", GT_RETURN, kCodeAutoFill));
    AddGenericEntry(Entry(TypeAssert, "object|id", GT_TYPE_ASSERT));
    AddGenericEntry(Entry(Continue, "", GT_CONTINUE));
    AddGenericEntry(Entry(Break, "", GT_BREAK));
    AddGenericEntry(Entry(Case, "object", GT_CASE));
    AddGenericEntry(Entry(When, "value", GT_WHEN, kCodeAutoSize));
  }

  void BasicUtilityRegister() {
    using namespace entry;
    AddEntry(Entry(WriteLog, kCodeNormalParm, "msg", "log"));
    AddEntry(Entry(Convert, kCodeNormalParm, "object", "convert"));
    AddEntry(Entry(Input, kCodeAutoFill, "msg", "input"));
    AddEntry(Entry(Print, kCodeNormalParm, kStrObject, "print"));
    AddEntry(Entry(TimeReport, kCodeNormalParm, "", "time"));
    AddEntry(Entry(Quit, kCodeNormalParm, "", "quit"));
    AddEntry(Entry(GetTypeId, kCodeNormalParm, "object", "typeid"));
    AddEntry(Entry(GetRawStringType, kCodeNormalParm, "object", "type"));
    AddEntry(Entry(Dir, kCodeNormalParm, "object", "dir"));
    AddEntry(Entry(Exist, kCodeNormalParm, "object|id", "exist"));
  }

  void Activiate() {
    using namespace entry;
    GenericRegister();
    BasicUtilityRegister();
    InitPlanners();
#if defined(_WIN32)
    InitLibraryHandler();
#endif
#if not defined(_DISABLE_SDL_)
    LoadSDLStuff();
#endif
  }
}
