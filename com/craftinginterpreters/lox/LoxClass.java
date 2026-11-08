package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

class LoxClass extends LoxInstance implements LoxCallable {
  final String name;
  final LoxClass superclass;
  private final Map<String, LoxFunction> methods;
  private final Map<String, LoxFunction> staticMethods;

  LoxClass(String name, LoxClass superclass, Map<String, LoxFunction> methods,
           Map<String, LoxFunction> staticMethods) {
    super(null);
    this.name = name;
    this.superclass = superclass;
    this.methods = methods;
    this.staticMethods = staticMethods;
  }

  @Override
  Object get(Token token, Interpreter interpreter) {
    LoxFunction method = staticMethods.get(token.lexeme);
    if (method != null) {
      if (method.isGetter()) return method.call(interpreter, java.util.Collections.emptyList());
      return method;
    }
    if (superclass != null) return superclass.get(token, interpreter);
    throw new RuntimeError(token, "Undefined property '" + token.lexeme + "'.");
  }

  @Override
  public int arity() {
    LoxFunction initializer = findMethod("init");
    return initializer == null ? 0 : initializer.arity();
  }

  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    LoxInstance instance = new LoxInstance(this);
    LoxFunction initializer = findMethod("init");
    if (initializer != null) initializer.bind(instance).call(interpreter, arguments);
    return instance;
  }

  LoxFunction findMethod(String name) {
    if (methods.containsKey(name)) return methods.get(name);
    if (superclass != null) return superclass.findMethod(name);
    return null;
  }

  @Override
  public String toString() {
    return name;
  }
}
