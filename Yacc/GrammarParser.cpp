//
// Created by tangny on 2021/10/16.
//

#include <string>
#include <iostream>
#include <cassert>
#include <fstream>
#include <set>
#include <algorithm>
#include <ctype.h>
#include "include/Yacc/GrammarParser.h"



int readToken(std::string &line, int &cur, Token &token) {
    token.clear();
    int len = line.length();
    while(cur < len && std::isspace(line[cur])) cur ++;
    if(cur >= len)
        return END_OF_LINE;

    while(cur < len && !std::isspace(line[cur]))
        token.append(1, line[cur++]);

    if(token.compare("%%") == 0)
        return END_OF_SEGMENT;

    if(token.compare(";") == 0)
        return END_OF_PRODUCER;

    if(token.compare(":") == 0 || token.compare("|") == 0)
        return NEXT_PRODUCTION;

    if(token.compare("%token") == 0)
        return TOKEN_LINE;

    if(token.compare("%start") == 0)
        return START_UNIT;

    if(token.compare("#listener") == 0 || token.compare("##") == 0)
        return ADD_LISTENER;

    if(token.compare("/*") == 0 || token.compare("//") == 0)
        return END_OF_LINE;

    if(token[0] == '\'') {
        assert(token.length() == 3);
        token = token[1];
    }

    return END_SUCCESS;
}

void writeListenerToFile(std::ofstream& ListenerTypes, std::string listener) {
    static std::set<std::string> occur;
    if(occur.find(listener) != occur.end()) return;
    occur.insert(listener);

    std::string listenerName = listener;
    std::transform(listenerName.begin(), listenerName.end(), listenerName.begin(), ::toupper);

    ListenerTypes << "#ifndef " << listenerName << std::endl;
    ListenerTypes << "#define " << listenerName << "(TYPE)" << " LISTEN(TYPE)" << std::endl;
    ListenerTypes << "#endif" << std::endl;
    ListenerTypes << listenerName << "(" << listener << ")" << std::endl;
    ListenerTypes << "#undef " << listenerName << std::endl;
    ListenerTypes << std::endl;
}

void parseProducer(GrammarSet &grammarSet, const char *filename) {
    std::string line;
    int cur = 0;
    int ret = 0;
    int part = 1, flag = 0;
    int changeLine = 0;

    // ?????????????????????????????? translation_unit
    int isStartUnit = 0;
    Token startUnit = "translation_unit";

    // ??????????????????
    int isListener = 0;
    int hasListener = 0;
    Token tempListener;

    std::set< Token > tokenExist;
    Token token;
    // ???????????????
    Token left;
    // ???????????????
    TokenArray right;
    right.resize(0);

    // ????????????????????????????????????
    std::ifstream YACC(filename);
    // ??????ListenerTypes.h?????????
    std::ofstream ListenerTypes("../include/AST/ListenerTypes.inc");

    if(!YACC) {
        std::cout << "Open yacc file failed";
        return;
    }

    if(!ListenerTypes) {
        std::cout << "Open listener file failed";
        return;
    }
    else {
        ListenerTypes << "// ??????????????????????????????????????????????????????" << std::endl;
        ListenerTypes << "// ??????????????????" << std::endl;
        ListenerTypes << std::endl;

        ListenerTypes << "#ifndef " << "LISTEN" << std::endl;
        ListenerTypes << "#define " << "LISTEN(TYPE)" << std::endl;
        ListenerTypes << "#endif" << std::endl;
        ListenerTypes << std::endl;
    }

    while(std::getline(YACC, line)) {
        cur = 0;
        changeLine = 0;
        while(true) {
            ret = readToken(line, cur, token);

            switch(ret) {
                // ????????????????????????????????????
                case(END_SUCCESS):
                    if(isStartUnit == 1) {
                        startUnit = token;
                        isStartUnit = 0;
                    }
                    else if(isListener == 1) {
                        writeListenerToFile(ListenerTypes, token);
                        tempListener = token;
                        isListener = 0;
                        hasListener = 1;
                    }
                    else if(part == 2) {
                        if (flag == 0)
                            left = token;
                        else
                            right.emplace_back(token);

                        if(tokenExist.find(token) == tokenExist.end()) {
                            tokenExist.insert(token);
                            grammarSet.addToken(token);
                        }
                    }
                    break;
                // ???????????????
                case(END_OF_LINE):
                    changeLine = 1;
                    break;
                // ????????????????????????
                case(END_OF_PRODUCER):
                    if(part == 2) {
                        if (grammarSet.getProducerCount() > 0) {
                            grammarSet.addProducer(left, right);
                            if (hasListener == 1) {
                                grammarSet.addListenerToCurrentProducer(tempListener);
                                hasListener = 0;
                            }
                        }
                    }
                    left.clear();
                    right.resize(0);
                    flag = 0;
                    changeLine = 1;
                    break;
                // ?????????????????????
                case(END_OF_SEGMENT):
                    part ++;
                    changeLine = 1;
                    break;
                // ??????????????????
                case(TOKEN_LINE):
                    changeLine = 1;
                    break;
                // ??????????????????
                case(START_UNIT):
                    isStartUnit = 1;
                    break;
                // ??????????????? (?????????????????????)
                case(ADD_LISTENER):
                    isListener = 1;
                    break;
                // ????????????????????????
                case(NEXT_PRODUCTION):
                    if(part == 2 && flag == 1) {
                        // printProducer(std::make_pair(left, right));
                        // ??????????????????
                        if (grammarSet.getProducerCount() == 0) {
                            TokenArray temp;
                            temp.emplace_back(startUnit);
                            grammarSet.addProducer(HYPER_STATEMENT, temp);
                        }
                        if (grammarSet.getProducerCount() > 0) {
                            grammarSet.addProducer(left, right);
                            if (hasListener == 1) {
                                grammarSet.addListenerToCurrentProducer(tempListener);
                                hasListener = 0;
                            }
                        }
                    }
                    right.resize(0);
                    flag = 1;
            }

            if(changeLine == 1) break;
        }
        if(part >= 3) break;
    }

    ListenerTypes << "#undef " << "LISTEN" << std::endl;
    ListenerTypes.close();
}