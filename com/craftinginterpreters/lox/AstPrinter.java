package com.craftinginterpreters.lox;

class AstPrinter implements Expr.Visitor<String> {
  String print(Expr expr) {
    return expr.accept(this);
  }

  @Override
  public String visitAssignExpr(Expr.Assign expr) {
    return parenthesize("= " + expr.name.lexeme, expr.value);
  }

  @Override
  public String visitBinaryExpr(Expr.Binary expr) {
    return parenthesize(expr.operator.lexeme, expr.left, expr.right);
  }

  @Override
  public String visitCallExpr(Expr.Call expr) {
    StringBuilder builder = new StringBuilder();
    builder.append("(call ").append(expr.callee.accept(this));
    for (Expr argument : expr.arguments) {
      builder.append(" ").append(argument.accept(this));
    }
    builder.append(")");
    return builder.toString();
  }

  @Override
  public String visitConditionalExpr(Expr.Conditional expr) {
    return parenthesize("?:", expr.condition, expr.thenBranch, expr.elseBranch);
  }

  @Override
  public String visitGroupingExpr(Expr.Grouping expr) {
    return parenthesize("group", expr.expression);
  }

  @Override
  public String visitGetExpr(Expr.Get expr) {
    return parenthesize("." + expr.name.lexeme, expr.object);
  }

  @Override
  public String visitSetExpr(Expr.Set expr) {
    return parenthesize("=" + expr.name.lexeme, expr.object, expr.value);
  }

  @Override
  public String visitThisExpr(Expr.This expr) {
    return expr.keyword.lexeme;
  }

  @Override
  public String visitSuperExpr(Expr.Super expr) {
    return "super." + expr.method.lexeme;
  }

  @Override
  public String visitLiteralExpr(Expr.Literal expr) {
    if (expr.value == null)
      return "nil";
    return expr.value.toString();
  }

  @Override
  public String visitVariableExpr(Expr.Variable expr) {
    return expr.name.lexeme;
  }

  @Override
  public String visitUnaryExpr(Expr.Unary expr) {
    return parenthesize(expr.operator.lexeme, expr.right);
  }

  private String parenthesize(String name, Expr... expressions) {
    StringBuilder builder = new StringBuilder();
    builder.append("(").append(name);
    for (Expr expression : expressions) {
      builder.append(" ").append(expression.accept(this));
    }
    builder.append(")");
    return builder.toString();
  }
}
