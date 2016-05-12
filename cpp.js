var hasOwnProperty = Object.prototype.hasOwnProperty;
var push = Array.prototype.push;
var unshift = Array.prototype.unshift;

var i = 0;
var WHITESPACE = ++i, IDENTIFIER = ++i, NUMBER = ++i, CHARACTER = ++i, STRING = ++i, PUNC = ++i, ERROR = ++i;
var PP_PUNCS = [
"%:%", "...", ">>=", "<<=", "->*", "##", "<:", ":>",
"<%", "%>", "%:", "::", ".*", "+=", "-=", "*=", "/=",
"%=", "^=", "&=", "|=", "<<", ">>", "==", "!=", "<=",
">=", "&&", "||", "++", "--", "->", "{", "}", "[",
"]", "#", "(", ")", ";", ":", "?", ".", "+", "-",
"*", "/", "%", "^", "&", "|", "~", "!", "=", "<", ">", ","];
var NON_MACRO_NAMES = ["defined", "__VA_ARGS__", "and", "and_eq", "bitand", "bitor", "compl", "not", "not_eq", "or", "or_eq", "xor", "xor_eq"];

function extend(base, extension) {
    for (var key in extension) {
        if (hasOwnProperty.call(extension, key)) {
            base[key] = extension[key];
        }
    }
}

function isAlpha(c) {
    return c >= "A" && c <= "Z" || c >= "a" && c <= "z";
}

function isDigit(c) {
    return c >= "0" && c <= "9";
}

function TokenStream() {}

TokenStream.prototype = {
    buffer: null,
    
    init: function () {
        this.buffer = [];
    },
    
    finished: function () {
        return this.buffer.length === 0;
    },
    
    next: function () {
        if (this.buffer.length > 0) {
            return this.buffer.shift();
        } else {
            return this._next();
        }
    },
    
    peek: function () {
        if (this.buffer.length > 0) {
            return this.buffer[0];
        } else {
            var token = this._next();
            this.buffer.push(token);
            return token;
        }
    },
    
    _next: function () {},
    
    space: function (allowNewLine) {
        var token = this.next();
        if (!token) {
            return null;
        } else if (token.type === WHITESPACE) {
            if (allowNewLine === false && token.hasNewLine) {
                this.buffer.unshift(token);
                return null;
            } else {
                return token;
            }
        } else {
            this.buffer.unshift(token);
            return null;
        }
    },
    
    expectNewLine: function () {
        var token = this.next();
        if (token && (token.type !== WHITESPACE || !token.hasNewLine)) {
            throw new Error("expected new line");
        }
        return token;
    },
    
    matchPunc: function (value) {
        var token = this.next();
        if (token && token.type === PUNC && token.value === value) {
            return token;
        } else {
            this.buffer.unshift(token);
            return null;
        }
    },
    
    expectPunc: function (value) {
        var token = this.next();
        if (token && token.type === PUNC && token.value === value) {
            return token;
        } else {
            throw new Error("expected " + value);
        }
    },
    
    matchId: function (value) {
        var token = this.next();
        if (token && token.type === IDENTIFIER && token.value === value) {
            return token;
        } else {
            this.buffer.unshift(token);
            return null;
        }
    },
    
    expectId: function () {
        var token = this.next();
        if (token && token.type === IDENTIFIER) {
            return token;
        } else {
            throw new Error("expected identifier");
        }
    },
    
    print: function (out) {
        var prev = null;
        while (token = this.next()) {
            if (prev) {
                if (prev.type === NUMBER &&
                        /^([0-9A-Za-z_\.]|'[0-9A-Za-z_])/.test(token.value) ||
                    prev.type === IDENTIFIER &&
                        /^[0-9A-Za-z_]/.test(token.value) ||
                    prev.type === PUNC &&
                        (PP_PUNCS.indexOf(prev.value + token.value.substr(0, 1)) >= 0 ||
                        PP_PUNCS.indexOf(prev.value + token.value.substr(0, 2)) >= 0 )) {
                    out(" ");
                }
            }
            out(token.value);
        }
    }
};


function Tokenizer() {}

Tokenizer.prototype = new TokenStream();

extend(Tokenizer.prototype, {
    text: "",
    pos: 0,
    length: 0,
    
    init: function (text) {
        TokenStream.prototype.init.call(this);
        this.text = text;
        this.length = text.length;
        this.pos = 0;
    },
    
    finished: function () {
        return this.pos >= this.length;
    },
    
    parseSpace: function () {
        var s = "", p = this.pos, c,
            hasNewLine = false;
        while (this.pos < this.length) {
            c = this.text.charAt(this.pos);
            if (c == "/") {
                c = this.text.charAt(this.pos + 1);
                if (c === "*") {
                    this.pos += 2;
                    s += " ";
                    while (this.pos < this.length) {
                        c = this.text.charAt(this.pos);
                        if (c === "*") {
                            c = this.text.charAt(++this.pos);
                            if (c == "/") {
                                ++this.pos;
                                break;
                            }
                        } else {
                            this.pos++;
                        }
                    }
                } else if (c === "/") {
                    this.pos += 2;
                    s += " ";
                    while (this.pos < this.length) {
                        c = this.text.charAt(this.pos);
                        if (c === "\r" || c === "\n") {
                            break;
                        } else {
                            this.pos++;
                        }
                    }
                } else {
                    break;
                }
            } else if (c === " " || c === "\t") {
                s += c;
                this.pos++;
            } else if (c === "\r" || c === "\n") {
                s += c;
                hasNewLine = true;
                this.pos++;
            } else if (c === "\\") {
                c = this.text.charAt(this.pos + 1);
                if (c === "\r") {
                    if (this.text.charAt(this.pos + 2) === "\n") {
                        this.pos += 3;
                        s += " ";
                    } else {
                        this.pos += 2;
                        s += " ";
                    }
                } else if (c === "\n") {
                    this.pos += 2;
                    s += " ";
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        if (this.pos > p) {
            return {
                type: WHITESPACE,
                value: s,
                pos: p,
                length: this.pos - p,
                hasNewLine: hasNewLine
            };
        } else {
            return null;
        }
    },
    
    parseIdent: function () {
        var p = this.pos;
        var c = this.text.charAt(++this.pos);
        while (isAlpha(c) || isDigit(c) || c === "_") {
            c = this.text.charAt(++this.pos);
        }
        var token = {
            type: IDENTIFIER,
            value: this.text.substring(p, this.pos),
            pos: p,
            length: this.pos - p
        };
        return token;
    },
    
    parseNumber: function () {
        var text = this.text,
        p = this.pos,
        c = text.charAt(this.pos);
        if (c === ".") {
            if (isDigit(text.charAt(this.pos+1))) {
                this.pos += 2;
            } else {
                return {
                    type: ERROR,
                    value: "expected digit",
                    pos: p,
                    length: 0
                };
            }
        } else if (isDigit(c)) {
            this.pos++;
        } else {
            return {
                type: ERROR,
                value: "expected digit",
                pos: p,
                length: 0
            };
        }
        while (this.pos < this.length) {
            c = text.charAt(this.pos);
            if (c === "E" || c === "e") {
                c = text.charAt(++this.pos);
                if (c === "+" || c === "-") {
                    this.pos++;
                }
            } else if (isDigit(c) || isAlpha(c) || c === "_" || c === ".") {
                this.pos++;
            } else if (c === "'") {
                c = text.charAt(++this.pos);
                if (isDigit(c) || isAlpha(c) || c === "_") {
                    this.pos++;
                } else {
                    throw new Error("unexpected");
                }
            } else {
                break;
            }
        }
        var token = {
            type: NUMBER,
            value: text.substring(p, this.pos),
            pos: p,
            length: this.pos - p
        };
        return token;
    },
    
    match: function (str) {
        var l = str.length;
        if (this.text.substr(this.pos, l) === str) {
            this.pos += l;
            return true;
        } else {
            return false;
        }
    },
    
    parsePunc: function () {
        var i, l = PP_PUNCS.length;
        var p = this.pos;
        for (i = 0; i<l; i++) {
            if (this.match(PP_PUNCS[i])) {
                return {
                    type: PUNC,
                    value: PP_PUNCS[i],
                    pos: p,
                    length: PP_PUNCS[i].length
                };
            }
        }
        this.pos++;
        return {
            type: PUNC,
            value: this.text.charAt(p),
            pos: p,
            length: 1
        };
    },
    
    hex: function () {
        var c = this.text.charAt(this.pos);
        if (c >= "0" && c <= "0" || c >= "A" && c <= "F" || c >= "a" && c <= "f") {
            this.pos++;
        } else {
            throw new Error("expected hexadecimal digit");
        }
    },
    
    parseEscape: function () {
        var c = this.text.charAt(this.pos);
        if (c === "'" || c === "\"" || c === "?" || c === "\\" ||
            c === "a" || c === "b" || c === "f" || c === "n" || c === "r" || c === "t" || c === "v") {
            this.pos++;
        } else if (c === "x" || c === "u") {
            this.pos++;
            this.hex();
            this.hex();
            this.hex();
            this.hex();
        } else if (c === "U") {
            this.hex();
            this.hex();
            this.hex();
            this.hex();
            this.hex();
            this.hex();
            this.hex();
            this.hex();
        } else if (c >= "0" && c <= "7") {
            c = this.text.charAt(++this.pos);
            if (c >= "0" && c <= "7") {
                c = this.text.charAt(++this.pos);
                if (c >= "0" && c <= "7") {
                    ++this.pos;
                }
            }
        }
    },
    
    parseCharSequence: function (quote, type, prefix) {
        var p = this.pos, c = this.text.charAt(this.pos);
        if (c !== quote) {
            return {
                type: ERROR,
                value: "expected quote",
                pos: this.pos,
                length: 0
            };
        }
        this.pos++;
        while (this.pos < this.length) {
            c = this.text.charAt(this.pos);
            if (c === "\\") {
                this.pos++;
                this.parseEscape();
            } else if (c === "\r" || c === "\n") {
                throw new Error("expected quote");
            } else if (c === quote) {
                this.pos++;
                return {
                    type: type,
                    value: prefix + this.text.substring(p, this.pos),
                    pos: p,
                    length: this.pos - p
                };
            } else {
                this.pos++;
            }
        }
        throw new Error("expected quote");
    },
    
    parseCharacterLiteral: function (prefix) {
        return this.parseCharSequence("'", CHARACTER, prefix);
    },
    
    parseStringLiteral: function (prefix) {
        return this.parseCharSequence("\"", STRING, prefix);
    },
    
    parseRawStringLiteral: function (prefix) {
        var indicator, p = this.pos, c;
        if (this.text.charAt(this.pos) !== "\"") {
            throw new Error("expected \"");
        }
        this.pos++;
        while (this.pos < this.length) {
            c = this.text.charAt(this.pos);
            if (c === "(") {
                indicator = ")" + this.text.substring(p+1, this.pos) + "\"";
                this.pos++;
                while (this.pos < this.length) {
                    if (this.match(indicator)) {
                        return {
                            type: STRING,
                            value: prefix + this.text.substring(p, this.pos),
                            pos: p,
                            length: this.pos - p
                        };
                    }
                    this.pos++;
                }
            } else if (c === " " || c === ")" || c === "\\" || c === "\t" || c === "\v" || c === "\f" || c === "\r" || c === "\n") {
                throw new Error("unexpected " + c);
            } else {
                this.pos++;
            }
        }
    },
    
    _next: function () {
        if (this.pos >= this.length)
            return null;
        var space = this.parseSpace();
        if (space !== null)
            return space;
        
        var c = this.text.charAt(this.pos);
        if (c === "." || isDigit(c)) {
            return this.parseNumber();
        } else if (c === "u" || c === "U" || c === "L" || c === "R") {
            var prefix = c, needString = false, isRaw = false;
            var p = this.pos + 1;
            var c2 = this.text.charAt(p);
            if (c === "R") {
                needString = true;
                isRaw = true;
            } else {
                if (c === "u") {
                    if (c2 === "8") {
                        needString = true;
                        p++;
                        prefix += c2;
                        c2 = this.text.charAt(p);
                    }
                }
                if (c2 === "R") {
                    needString = true;
                    isRaw = true;
                    p++;
                    prefix += c2;
                    c2 = this.text.charAt(p);
                }
            }
            if (c2 === "\"") {
                this.pos = p;
                if (isRaw) {
                    return this.parseRawStringLiteral(prefix);
                } else {
                    return this.parseStringLiteral(prefix);
                }
            } else if (c2 === "'") {
                this.pos = p;
                if (needString) {
                    throw new Error("expected \"");
                }
                return this.parseCharacterLiteral(prefix);
            } else {
                return this.parseIdent();
            }
        } else if (c === "\"") {
            return this.parseStringLiteral("");
        } else if (c === "'") {
            return this.parseCharacterLiteral("");
        } else if (isAlpha(c)) {
            return this.parseIdent();
        }
        return this.parsePunc();
    }
});

function MacroExpander() {}

MacroExpander.prototype = new TokenStream();

extend(MacroExpander.prototype, {
    input: null,
    macroTable: null,
    expander: null,
    stack: null,
    
    init: function (input, macroTable, stack) {
        TokenStream.prototype.init.call(this);
        this.input = input;
        this.macroTable = macroTable;
        this.expander = null;
        this.stack = stack || [];
    },
    
    expandObjectLikeMacro: function (macro) {
        if (macro.body.length > 0) {
            var stream = new TokenStream();
            stream.buffer = macro.body;
            var expander = new MacroExpander();
            expander.init(stream, this.macroTable, this.stack.concat(macro.name));
            this.expander = expander;
        }
        return this._next();
    },
    
    expandFunctionLikeMacro: function (name, macro) {
        var args = [], curArg = [];
        var input = this.input, depth, token = input.space();
        if (input.matchPunc("(")) {
            depth = 0;
            var stream = new TokenStream();
            var expander = new MacroExpander();
            expander.init(stream, this.macroTable, this.stack);
            while (!input.finished()) {
                if (token = input.matchPunc(")")) {
                    if (depth === 0) {
                        stream.buffer = curArg;
                        expander.init(stream, this.macroTable);
                        curArg = [];
                        while (token = expander.next()) {
                            curArg.push(token);
                        }
                        args.push(curArg);
                        
                        stream.buffer = this.substituteArgs(macro.body, this.matchArgs(macro.params, args));
                        expander.init(stream, this.macroTable, this.stack.concat(macro.name));
                        this.expander = expander;
                        return this._next();
                    } else {
                        depth--;
                        curArg.push(token);
                    }
                } else if (token = input.matchPunc("(")) {
                    depth++;
                    curArg.push(token);
                } else if (depth === 0 && input.matchPunc(",")) {
                    stream.buffer = curArg;
                    expander.init(stream, this.macroTable);
                    curArg = [];
                    while (token = expander.next()) {
                        curArg.push(token);
                    }
                    args.push(curArg);
                } else {
                    curArg.push(input.next());
                }
            }
        } else {
            this.buffer.unshift(token);
            return name;
        }
    },
    
    matchArgs: function (params, args) {
        var i, l = params.length, argsTable = {};
        if (params[l-1] === "__VA_ARGS__") {
            if (args.length >= l-1) {
                var __VA_ARGS__ = [];
                for (i = l-1; i<args.length; i++) {
                    if (i > l-1) {
                        __VA_ARGS__.push({
                            type: PUNC,
                            value: ",",
                            pos: 0,
                            length: 1
                        });
                    }
                    push.apply(__VA_ARGS__, args[i]);
                }
                args[l-1] = __VA_ARGS__;
            } else {
                throw new Error("expected at least " + (l-1) + " arguments, but got " + args.length);
            }
        } else if (l === 0 && args.length === 1 && args[0].type === WHITESPACE) {
            return argsTable;
        } else if (args.length !== l) {
            throw new Error("expected " + l + " arguments, but got " + args.length);
        }
        for (i = 0; i<l; i++) {
            argsTable[params[i]] = args[i];
        }
        return argsTable;
    },
    
    substituteArgs: function (body, args) {
        var replacedBody = [], i, l = body.length, token;
        for (i = 0; i<l; i++) {
            token = body[i];
            if (token.type === IDENTIFIER && hasOwnProperty.call(args, token.value)) {
                push.apply(replacedBody, args[token.value]);
            } else {
                replacedBody.push(token);
            }
        }
        return replacedBody;
    },
    
    expandMacro: function (name) {
        var input = this.input, macro, arg;
        
        if (name.value === "__VA_ARGS") {
            throw new Error("unexpected __VA_ARGS__");
        }
        
        if (hasOwnProperty.call(this.macroTable, name.value) &&
            this.stack.indexOf(name.value) === -1) {
            macro = this.macroTable[name.value];
            if (!macro.isFunctionLike) {
                return this.expandObjectLikeMacro(macro);
            } else {
                return this.expandFunctionLikeMacro(name, macro);
            }
        }
        return name;
    },
    
    _next: function () {
        var expander = this.expander;
        var token;
        if (expander !== null) {
            token = this.expander.next();
            if (token === null) {
                this.expander = null;
            } else {
                return token;
            }
        }
        
        var input = this.input;
        if (input.finished())
            return null;
        var token = input.next();
        if (token.type === IDENTIFIER) {
            return this.expandMacro(token);
        } else {
            return token;
        }
    }
});

function truncateNewLine(token) {
    var space = token.value.match(/(\r\n|\r|\n)[^\r\n]*$/)[0];
    return {
        type: WHITESPACE,
        value: space,
        pos: token.pos + token.length - space.length,
        length: space.length,
        hasNewLine: true
    };
}

function FilePreprocessor() {}

FilePreprocessor.prototype = new MacroExpander();

extend(FilePreprocessor.prototype, {
    macroTable: null,
    ifStack: null,

    init: function (content, macroTable) {
        var tokenizer = new Tokenizer();
        tokenizer.init(content);
        MacroExpander.prototype.init.call(this, tokenizer, macroTable);
        this.ifStack = [];
    },
    
    skipUntilNewLine: function () {
        var token;
        while (token = this.input.next()) {
            if (token.type === WHITESPACE && token.hasNewLine) {
                return true;
            }
        }
        return false;
    },
    
    untilNewLine: function (allowVAARGS) {
        var tokens = [], input = this.input, token;
        while (token = input.next()) {
            if (token.type === WHITESPACE && token.hasNewLine) {
                input.buffer.unshift(token);
                return tokens;
            } else if (allowVAARGS === false && token.type === IDENTIFIER && token.value === "__VA_ARGS__") {
                throw new Error("unexpected __VA_ARGS__");
            } else {
                tokens.push(token);
            }
        }
        if (tokens.length > 0 && tokens[tokens.length - 1].type === WHITESPACE) {
            tokens.pop();
        }
        return tokens;
    },
    
    parseFunctionMacroTail: function (name, params, allowVAARGS) {
        var body = this.untilNewLine(false);
        var macro = new FunctionMacro();
        macro.init(name, params, body);
        this.macroTable[name] = macro;
        return this.input.expectNewLine();
    },
    
    parseDefine: function () {
        var input = this.input, token, name, body, macro;
        token = input.expectId();
        name = token.value;
        if (NON_MACRO_NAMES.indexOf(name) >= 0) {
            throw new Error("Invalid macro name: " + name);
        }
        if (input.matchPunc("(")) {
            var first = true, params = [];
            while (!input.finished()) {
                input.space(false);
                if (input.matchPunc(")")) {
                    return this.parseFunctionMacroTail(name, params, false);
                } else {
                    if (first) {
                        first = false;
                    } else {
                        input.expectPunc(",");
                        input.space(false);
                    }
                    if (input.matchPunc("...")) {
                        params.push("__VA_ARGS__");
                        input.space(false);
                        input.expectPunc(")");
                        return this.parseFunctionMacroTail(name, params, true);
                    } else {
                        token = input.expectId();
                        params.push(token.value);
                    }
                }
            }
            throw new Error("expected )");
        } else {
            token = input.space(false);
            if (!token) {
                throw new Error("expected space");
            }
            body = this.untilNewLine();
            macro = new Macro();
            macro.init(name, body);
            this.macroTable[name] = macro;
            return input.expectNewLine();
        }
    },
    
    parseCondition: function () {
        
    },
    
    parseIf: function (skip) {
        input.space(false);
        var cond = this.parseCondition(),
        token = this.input.expectNewLine();
        
        if (skip) return this.skipIfElseClauses(1);
        if (cond) {
            this.ifStack.push(1);
            return truncateNewLine(token);
        } else {
            return this.skipIfElseClauses(2);
        }
    },
    
    parseIfdef: function (negate, skip) {
        var input = this.input, token, name, ifStack = this.ifStack;
        
        input.space(false);
        name = input.expectId().value;
        token = input.expectNewLine();
        
        if (skip) return this.skipIfElseClauses(1);
        if (hasOwnProperty.call(this.macroTable, name) == !negate) {
            ifStack.push(1);
            return truncateNewLine(token);
        } else {
            return this.skipIfElseClauses(2);
        }
    },
    
    skipIfElseClauses: function (level) {
        var token, input = this.input, cond;
        while (true) {
            if (input.matchPunc("#")) {
                input.space(false);
                if (input.matchId("if")) {
                    this.parseIf(true);
                } else if (input.matchId("ifdef") || input.matchId("ifndef")) {
                    this.parseIfdef(true, true);
                } else if (input.matchId("else")) {
                    if (level === 0) {
                        throw new Error("unexpected #elif");
                    }
                    token = input.expectNewLine();
                    if (level === 2) {
                        this.ifStack.push(2);
                        return truncateNewLine(token);
                    }
                } else if (input.matchId("elif")) {
                    if (level === 0) {
                        throw new Error("unexpected #elif");
                    }
                    input.space(false);
                    cond = this.parseCondition();
                    token = input.expectNewLine();
                    if (level === 2 && cond) {
                        this.ifStack.push(1);
                        return truncateNewLine(token);
                    }
                } else if (input.matchId("endif")) {
                    return truncateNewLine(input.expectNewLine());
                } else {
                    this.skipUntilNewLine();
                }
            } else if (!this.skipUntilNewLine()) {
                return null;
            }
        }
    },
    
    _next: function () {
        var expander = this.expander, token, input;
        if (expander !== null) {
            token = this.expander.next();
            if (token === null) {
                this.expander = null;
            } else {
                return token;
            }
        }
        
        input = this.input;
        if (input.finished())
            return null;
        token = input.space();
        if (token)
            return token;
        
        if (input.matchPunc("#")) {
            input.space(false);
            if (input.matchId("define")) {
                input.space(false);
                return this.parseDefine();
            } else if (input.matchId("if")) {
                return this.parseIf();
            } else if (input.matchId("ifdef")) {
                return this.parseIfdef();
            } else if (input.matchId("ifndef")) {
                return this.parseIfdef(true);
            } else if (input.matchId("elif")) {
                if (this.ifStack.length === 0 || this.ifStack.pop() !== 1) {
                    throw new Error("unexpected #elif");
                }
                this.parseCondition();
                input.expectNewLine();
                return this.skipIfElseClauses(1);
            } else if (input.matchId("else")) {
                if (this.ifStack.length === 0 || this.ifStack.pop() !== 1) {
                    throw new Error("unexpected #else");
                }
                input.expectNewLine();
                return this.skipIfElseClauses(0);
            } else if (input.matchId("endif")) {
                if (this.ifStack.length === 0) {
                    throw new Error("unexpected #endif");
                }
                this.ifStack.pop();
                token = input.expectNewLine();
                return truncateNewLine(token);
            }
        } else {
            token = input.next();
            if (token && token.type === IDENTIFIER) {
                return this.expandMacro(token);
            } else {
                return token;
            }
        }
    }
});

function Macro() {}

Macro.prototype = {
    isFunctionLike: false,
    name: null,
    body: null,
    
    init: function (name, body) {
        this.name = name;
        this.body = body;
    }
};

function FunctionMacro() {}

FunctionMacro.prototype = new Macro();

extend(FunctionMacro.prototype, {
    isFunctionLike: true,
    params: null,
    
    init: function (name, params, body) {
        Macro.prototype.init.call(this, name, body);
        this.params = params;
    }
});

if (typeof exports === "undefined")
    exports = {};

exports.tokenTypes = {
    WHITESPACE: WHITESPACE,
    IDENTIFIER: IDENTIFIER,
    NUMBER: NUMBER,
    CHARACTER: CHARACTER,
    STRING: STRING,
    PUNC: PUNC,
    ERROR: ERROR
};

exports.TokenStream = TokenStream;
exports.Tokenizer = Tokenizer;
exports.MacroExpander = MacroExpander;
exports.FilePreprocessor = FilePreprocessor;
exports.Macro = Macro;
exports.FunctionMacro = FunctionMacro;

if (typeof require === "function") {
    var fs = require("fs");
    exports.processFile = function (path) {
        var content = fs.readFileSync(path);
        var fp = new FilePreprocessor();
        fp.init(content, {});
        fp.print(function (str) {
            process.stdout.write(str);
        });
    };
}

function assert(cond, msg) {
    if (!cond)
        throw new Error("Assertion failed: " + (msg || ""));
}

function testTokenizer() {
    var t = new Tokenizer(), tok;
    t.init("a/*foo\n*/b //\n");
    tok = t.next();
    assert(tok.type === IDENTIFIER);
    assert(tok.value === "a");
    tok = t.next();
    assert(tok.type === WHITESPACE);
    assert(tok.value === " ");
    assert(!tok.hasNewLine);
    tok = t.next();
    assert(tok.type === IDENTIFIER);
    assert(tok.value === "b");
    tok = t.next();
    assert(tok.type === WHITESPACE);
    assert(tok.value === "  \n");
    assert(tok.hasNewLine);
    assert(t.next() === null);
    
    t.init("''u'cd'U'\\000''\\n'U\"aaaa\\\"\"u8R\"/*(\nfoo)/*\"");
    tok = t.next();
    assert(tok.type === CHARACTER);
    assert(tok.value === "''");
    tok = t.next();
    assert(tok.type === CHARACTER);
    assert(tok.value === "u'cd'");
    tok = t.next();
    assert(tok.type === CHARACTER);
    assert(tok.value === "U'\\000'");
    tok = t.next();
    assert(tok.type === CHARACTER);
    assert(tok.value === "'\\n'");
    tok = t.next();
    assert(tok.type === STRING);
    assert(tok.value === "U\"aaaa\\\"\"");
    tok = t.next();
    assert(tok.type === STRING);
    assert(tok.value === "u8R\"/*(\nfoo)/*\"");
    assert(t.next() === null);
}

function testMacroExpansion() {
    var stream = new Tokenizer();
    stream.init("foo");
    
    var macro = new Macro();
    macro.init("foo", [{
        type: NUMBER,
        value: "123",
        pos: 0,
        length: 3
    }]);
    
    var expander = new MacroExpander();
    expander.init(stream, {
        foo: macro
    });
    
    var token = expander.next();
    assert(token.type === NUMBER);
    assert(token.value === "123");
    assert(expander.next() === null);
}

function testFile() {
    var content = "#define a(x) b(x) + 1\n#define b(x) x + 2\na(0)\na (0)\nb(b(0))\n#define c(x) c(x + 1)\nc(0)\n\nconst int x = 1;\n#if x\nfoo\n#endif\n\n#define e \\\\\nfoo\ne\n\n#define f (0)\n#define g() a\ng()f\n#define g2(x, y) x y\ng2(g(), f)\n\n#define h(x) x(0)\nh(a)\n\n#define i(x) k(x)\n#define j 1,2\n#define k(x, y) x foo y\ni(j)\ng2(^s#  0\\, *- sa:_ $)\ng2([(]0), ((}}1)){)\ng2( , )\n#define l() 0\nl( )\n#define m(a, ...) a + __VA_ARGS__\nm(1, 2, 3)\n";
    var fp = new FilePreprocessor();
    var buffer = [];
    fp.init(content, {});
    fp.print(function (str) {
        buffer.push(str);
    });
    console.log(buffer.join(""));
}

