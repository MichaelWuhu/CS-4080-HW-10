 package com.craftinginterpreters.lox;

import java.util.List;
import java.util.HashMap;
import java.util.Map;

 class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Void> {
   private static class BreakException extends RuntimeException {
     private static final long serialVersionUID = 1L;
   }

   final Environment globals = new Environment();
   private Environment environment = globals;
   private boolean replMode = false;
   private final Map<Expr, Integer> locals = new HashMap<>();

   Interpreter() {
     globals.define("clock", new LoxCallable() {
       @Override
       public int arity() { return 0; }

       @Override
       public Object call(Interpreter interpreter, List<Object> arguments) {
         return (double) System.currentTimeMillis() / 1000.0;
       }

       @Override
       public String toString() { return "<native fn>"; }
     });
   }

   void interpret(List<Stmt> statements, boolean replMode) {
     this.replMode = replMode;
     try {
       for (Stmt statement : statements) {
         if (statement != null) execute(statement);
       }
     } catch (RuntimeError error) {
       Lox.runtimeError(error);
     } finally {
       this.replMode = false;
     }
   }

   private void execute(Stmt stmt) {
     stmt.accept(this);
   }

   void executeBlock(List<Stmt> statements, Environment blockEnvironment) {
     Environment previous = environment;
     try {
       environment = blockEnvironment;
       for (Stmt statement : statements) {
         if (statement != null) execute(statement);
       }
     } finally {
       environment = previous;
     }
   }

   void resolve(Expr expr, int depth) {
     locals.put(expr, depth);
   }

   @Override
   public Void visitBlockStmt(Stmt.Block stmt) {
     executeBlock(stmt.statements, new Environment(environment));
     return null;
   }

   @Override
   public Void visitAssertStmt(Stmt.Assert stmt) {
     if (!isTruthy(evaluate(stmt.condition))) {
       throw new RuntimeError(stmt.keyword, "Assertion failed.");
     }
     return null;
   }

   @Override
   public Void visitBreakStmt(Stmt.Break stmt) {
     throw new BreakException();
   }

   @Override
   public Void visitClassStmt(Stmt.Class stmt) {
     LoxClass superclass = null;
     if (stmt.superclass != null) {
       Object value = evaluate(stmt.superclass);
       if (!(value instanceof LoxClass)) {
         throw new RuntimeError(stmt.superclass.name, "Superclass must be a class.");
       }
       superclass = (LoxClass) value;
     }
     environment.define(stmt.name.lexeme, null);
     if (superclass != null) {
       environment = new Environment(environment);
       environment.define("super", superclass);
     }
     Map<String, LoxFunction> methods = new HashMap<>();
     Map<String, LoxFunction> staticMethods = new HashMap<>();
     for (Stmt.Function method : stmt.methods) {
       LoxFunction function = new LoxFunction(method, environment,
           method.name.lexeme.equals("init"));
       if (method.isStatic) staticMethods.put(method.name.lexeme, function);
       else methods.put(method.name.lexeme, function);
     }
     LoxClass klass = new LoxClass(stmt.name.lexeme, superclass, methods, staticMethods);
     if (superclass != null) environment = environment.enclosing;
     environment.assign(stmt.name, klass);
     return null;
   }

   @Override
   public Void visitFunctionStmt(Stmt.Function stmt) {
     LoxFunction function = new LoxFunction(stmt, environment);
     environment.define(stmt.name.lexeme, function);
     return null;
   }

   @Override
   public Void visitExpressionStmt(Stmt.Expression stmt) {
     Object value = evaluate(stmt.expression);
     if (replMode) System.out.println(stringify(value));
     return null;
   }

   @Override
   public Void visitIfStmt(Stmt.If stmt) {
     if (isTruthy(evaluate(stmt.condition))) {
       execute(stmt.thenBranch);
     } else if (stmt.elseBranch != null) {
       execute(stmt.elseBranch);
     }
     return null;
   }

   @Override
   public Void visitPrintStmt(Stmt.Print stmt) {
     System.out.println(stringify(evaluate(stmt.expression)));
     return null;
   }

   @Override
   public Void visitReturnStmt(Stmt.Return stmt) {
     Object value = null;
     if (stmt.value != null) value = evaluate(stmt.value);
     throw new Return(value);
   }

   @Override
   public Void visitVarStmt(Stmt.Var stmt) {
     if (stmt.initializer == null) {
       environment.defineUninitialized(stmt.name.lexeme);
     } else {
       environment.define(stmt.name.lexeme, evaluate(stmt.initializer));
     }
     return null;
   }

   @Override
   public Void visitWhileStmt(Stmt.While stmt) {
     try {
       while (isTruthy(evaluate(stmt.condition))) {
         execute(stmt.body);
       }
     } catch (BreakException breakException) {
       // Exit only the nearest loop.
     }
     return null;
   }

   @Override
   public Object visitAssignExpr(Expr.Assign expr) {
     Object value = evaluate(expr.value);
     Integer distance = locals.get(expr);
     if (distance != null) {
       environment.assignAt(distance, expr.name, value);
     } else {
       globals.assign(expr.name, value);
     }
     return value;
   }

   @Override
   public Object visitVariableExpr(Expr.Variable expr) {
     return lookUpVariable(expr.name, expr);
   }

   private Object lookUpVariable(Token name, Expr expr) {
     Integer distance = locals.get(expr);
     if (distance != null) {
       return environment.getAt(distance, name.lexeme);
     }
     return globals.get(name);
   }

   @Override
   public Object visitLiteralExpr(Expr.Literal expr) {
     return expr.value;
   }

   @Override
   public Object visitGroupingExpr(Expr.Grouping expr) {
     return evaluate(expr.expression);
   }

   @Override
   public Object visitUnaryExpr(Expr.Unary expr) {
     Object right = evaluate(expr.right);
     switch (expr.operator.type) {
       case BANG:
         return !isTruthy(right);
       case MINUS:
         checkNumberOperand(expr.operator, right);
         return -(double) right;
       default:
         return null;
     }
   }

   @Override
   public Object visitBinaryExpr(Expr.Binary expr) {
     Object left = evaluate(expr.left);

     switch (expr.operator.type) {
       case AND:
         if (!isTruthy(left)) return left;
         return evaluate(expr.right);
       case OR:
         if (isTruthy(left)) return left;
         return evaluate(expr.right);
       case COMMA:
         return evaluate(expr.right);
       case GREATER:
         return compare(expr.operator, left, evaluate(expr.right)) > 0;
       case GREATER_EQUAL:
         return compare(expr.operator, left, evaluate(expr.right)) >= 0;
       case LESS:
         return compare(expr.operator, left, evaluate(expr.right)) < 0;
       case LESS_EQUAL:
         return compare(expr.operator, left, evaluate(expr.right)) <= 0;
       case MINUS:
         return numericBinary(expr, left, evaluate(expr.right), '-');
       case PLUS: {
         Object right = evaluate(expr.right);
         if (left instanceof Double && right instanceof Double) {
           return (double) left + (double) right;
         }
         if (left instanceof String || right instanceof String) {
           return stringify(left) + stringify(right);
         }
         throw new RuntimeError(expr.operator,
             "Operands must be two numbers or include a string.");
       }
       case SLASH: {
         Object right = evaluate(expr.right);
         checkNumberOperands(expr.operator, left, right);
         if ((double) right == 0.0) {
           throw new RuntimeError(expr.operator, "Cannot divide by zero.");
         }
         return (double) left / (double) right;
       }
       case STAR:
         return numericBinary(expr, left, evaluate(expr.right), '*');
       case BANG_EQUAL:
         return !isEqual(left, evaluate(expr.right));
       case EQUAL_EQUAL:
         return isEqual(left, evaluate(expr.right));
       default:
         return null;
     }
   }

   @Override
   public Object visitCallExpr(Expr.Call expr) {
     Object callee = evaluate(expr.callee);

     List<Object> arguments = new java.util.ArrayList<>();
     for (Expr argument : expr.arguments) {
       arguments.add(evaluate(argument));
     }

     if (!(callee instanceof LoxCallable)) {
       throw new RuntimeError(expr.paren, "Can only call functions and classes.");
     }

     LoxCallable function = (LoxCallable) callee;
     if (arguments.size() != function.arity()) {
       throw new RuntimeError(expr.paren, "Expected " + function.arity()
           + " arguments but got " + arguments.size() + ".");
     }

     return function.call(this, arguments);
   }

   @Override
   public Object visitGetExpr(Expr.Get expr) {
     Object object = evaluate(expr.object);
     if (object instanceof LoxClass) return ((LoxClass) object).get(expr.name, this);
     if (object instanceof LoxInstance) return ((LoxInstance) object).get(expr.name, this);
     throw new RuntimeError(expr.name, "Only instances and classes have properties.");
   }

   @Override
   public Object visitSetExpr(Expr.Set expr) {
     Object object = evaluate(expr.object);
     if (!(object instanceof LoxInstance)) {
       throw new RuntimeError(expr.name, "Only instances have fields.");
     }
     Object value = evaluate(expr.value);
     ((LoxInstance) object).set(expr.name, value);
     return value;
   }

   @Override
   public Object visitThisExpr(Expr.This expr) {
     return lookUpVariable(expr.keyword, expr);
   }

   @Override
   public Object visitSuperExpr(Expr.Super expr) {
     int distance = locals.get(expr);
     LoxClass superclass = (LoxClass) environment.getAt(distance, "super");
     LoxInstance object = (LoxInstance) environment.getAt(distance - 1, "this");
     LoxFunction method = superclass.findMethod(expr.method.lexeme);
     if (method == null) {
       throw new RuntimeError(expr.method,
           "Undefined property '" + expr.method.lexeme + "'.");
     }
     return method.bind(object);
   }

   private Object numericBinary(Expr.Binary expr, Object left, Object right, char operator) {
     checkNumberOperands(expr.operator, left, right);
     if (operator == '-') return (double) left - (double) right;
     return (double) left * (double) right;
   }

   @Override
   public Object visitConditionalExpr(Expr.Conditional expr) {
     if (isTruthy(evaluate(expr.condition))) return evaluate(expr.thenBranch);
     return evaluate(expr.elseBranch);
   }

   private Object evaluate(Expr expr) {
     return expr.accept(this);
   }

   private boolean isTruthy(Object object) {
     if (object == null) return false;
     if (object instanceof Boolean) return (boolean) object;
     return true;
   }

   private boolean isEqual(Object a, Object b) {
     if (a == null && b == null) return true;
     if (a == null) return false;
     return a.equals(b);
   }

   private int compare(Token operator, Object left, Object right) {
     if (left instanceof Double && right instanceof Double) {
       return Double.compare((double) left, (double) right);
     }
     if (left instanceof String && right instanceof String) {
       return ((String) left).compareTo((String) right);
     }
     throw new RuntimeError(operator,
         "Operands must be two numbers or two strings.");
   }

   private void checkNumberOperand(Token operator, Object operand) {
     if (operand instanceof Double) return;
     throw new RuntimeError(operator, "Operand must be a number.");
   }

   private void checkNumberOperands(Token operator, Object left, Object right) {
     if (left instanceof Double && right instanceof Double) return;
     throw new RuntimeError(operator, "Operands must be numbers.");
   }

   private String stringify(Object object) {
     if (object == null) return "nil";
     if (object instanceof Double) {
       String text = object.toString();
       if (text.endsWith(".0")) text = text.substring(0, text.length() - 2);
       return text;
     }
     return object.toString();
   }
 }
