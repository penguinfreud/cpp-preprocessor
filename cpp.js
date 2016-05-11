var hasOwnProperty = Object.prototype.hasOwnProperty;
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
var NON_MACRO_NAMES = ["defined", "and", "and_eq", "bitand", "bitor", "compl", "not", "not_eq", "or", "or_eq", "xor", "xor_eq"];

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
            return buffer.shift();
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
            return token
        }
    },
    
    _next: function () {},
    
    space: function () {
        var token = this.next();
        if (token.type === WHITESPACE) {
            return token;
        } else {
            buffer.unshift(token);
            return null;
        }
    },
    
    matchPunc: function (value) {
        var token = this.next();
        if (token && token.type === PUNC && token.value === value) {
            return token;
        } else {
            buffer.unshift(token);
            return null;
        }
    },
    
    matchId: function (value) {
        var token = this.next();
        if (token && token.type === IDENTIFIER && token.value === value) {
            return token;
        } else {
            buffer.unshift(token);
            return null;
        }
    },
    
    expectId: function () {
        var token = this.next();
        if (token.type !== IDENTIFIER)
            throw new Error("expected identifier");
        return token;
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

function MacroExpansionStream() {}

MacroExpansionStream.prototype = new TokenStream();

extend(MacroExpansionStream.prototype, {
    input: null,
    macroTable: null,
    expandingStream: null,
    
    init: function (input, macroTable) {
        this.input = input;
        this.macroTable = macroTable;
    },
    
    expandMacro: function (name) {
        var input = this.input, stream, token;
        
        if (hasOwnProperty.call(this.macroTable, name.value)) {
            var macro = this.macroTable[name.value];
            if (!macro.isFunctionLike) {
                if (macro.body.length === 0) {
                    return this._next();
                }
                stream = new TokenStream();
                stream.buffer = macro.body;
                this.expandingStream = new MacroExpansionStream();
                this.expandingStream.init(stream, this.macroTable);
                token = this.expandingStream.next();
                if (token === null) {
                    return this._next();
                }
            } else {
                var buffer = [], params = [];
                if (token = input.space()) {
                    buffer.push(token);
                }
                if (token = input.matchPunc("(")) {
                    buffer.push(token);
                    while (!input.finished()) {
                        if (input.matchPunc(")")) {
                            var newMacroTable = Object.create(this.macroTable);
                        }
                    }
                    unshift.apply(input, buffer);
                } else {
                    unshift.apply(input, buffer);
                }
            }
        }
        return token;
    },
    
    _next: function () {
        var expandingStream = this.expandingStream;
        if (expandingStream !== null) {
            if (expandingStream.finished()) {
                this.expandingStream = null;
            } else {
                return this.expandingStream.next();
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

function FileStream() {}

FileStream.prototype = new MacroExpansionStream();

extend(FileStream.prototype, {
    tokenizer: null,
    macroTable: null,

    init: function (content, macroTable) {
        var tokenizer = new Tokenizer();
        tokenizer.init(content);
        MacroExpansionStream.prototype.init.call(tokenizer, macroTable);
    },
    
    parseDefine: function () {
        
    },
    
    parseIf: function () {
    
    },
    
    parseIfdef: function () {
    
    },
    
    _next: function () {
        var tokenizer = this.tokenzier, token;
        if (tokenzier.finished())
            return null;
        var token = tokenizer.space();
        if (token)
            return token;
        
        if (tokenizer.matchPunc("#")) {
            tokenizer.space();
            if (tokenizer.matchId("define")) {
                tokenizer.space();
                this.parseDefine();
            } else if (tokenizer.matchId("if")) {
                this.parseIf();
            } else if (tokenizer.matchId("ifdef")) {
                this.parseIfdef();
            }
        } else {
            token = tokenizer.next();
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

