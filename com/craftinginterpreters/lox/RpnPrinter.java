package com.craftinginterpreters.lox;

public class RpnPrinter implements Expr.Visitor<String> {
  String print(Expr expr) {
    return expr.accept(this);
  }

  @Override
  public String visitAssignExpr(Expr.Assign expr) {
    return expr.value.accept(this) + " " + expr.name.lexeme + " =";
  }

  @Override
  public String visitBinaryExpr(Expr.Binary expr) {
    return expr.left.accept(this) + " "
        + expr.right.accept(this) + " "
        + expr.operator.lexeme;
  }

  @Override
  public String visitCallExpr(Expr.Call expr) {
    StringBuilder builder = new StringBuilder(expr.callee.accept(this));
    for (Expr argument : expr.arguments) {
      builder.append(" ").append(argument.accept(this));
    }
    return builder.append(" call").toString();
  }

  @Override
  public String visitConditionalExpr(Expr.Conditional expr) {
    return expr.condition.accept(this) + " "
        + expr.thenBranch.accept(this) + " "
        + expr.elseBranch.accept(this) + " ?:";
  }

  @Override
  public String visitGroupingExpr(Expr.Grouping expr) {
    return expr.expression.accept(this);
  }

  @Override
  public String visitGetExpr(Expr.Get expr) {
    return expr.object.accept(this) + " " + expr.name.lexeme + ".";
  }

  @Override
  public String visitSetExpr(Expr.Set expr) {
    return expr.object.accept(this) + " " + expr.value.accept(this) + " " + expr.name.lexeme + "=";
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
    String operator = expr.operator.lexeme;
    if (expr.operator.type == TokenType.MINUS)
      operator = "~";
    return expr.right.accept(this) + " " + operator;
  }

  public static void main(String[] args) {
    Expr expression = new Expr.Binary(
        new Expr.Grouping(
            new Expr.Binary(
                new Expr.Literal(1),
                new Token(TokenType.PLUS, "+", null, 1),
                new Expr.Literal(2))),
        new Token(TokenType.STAR, "*", null, 1),
        new Expr.Grouping(
            new Expr.Binary(
                new Expr.Literal(4),
                new Token(TokenType.MINUS, "-", null, 1),
                new Expr.Literal(3))));

    System.out.println(new RpnPrinter().print(expression));
  }
}
