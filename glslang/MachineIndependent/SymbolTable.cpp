//
//Copyright (C) 2002-2005  3Dlabs Inc. Ltd.
//Copyright (C) 2012-2013 LunarG, Inc.
//
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//
//    Neither the name of 3Dlabs Inc. Ltd. nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
//COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//POSSIBILITY OF SUCH DAMAGE.
//

//
// Symbol table for parsing.  Most functionaliy and main ideas
// are documented in the header file.
//

#include "SymbolTable.h"

namespace glslang {

//
// TType helper function needs a place to live.
//

//
// Recursively generate mangled names.
//
void TType::buildMangledName(TString& mangledName)
{
    if (isMatrix())
        mangledName += 'm';
    else if (isVector())
        mangledName += 'v';

    switch (basicType) {
    case EbtFloat:              mangledName += 'f';      break;
    case EbtDouble:             mangledName += 'd';      break;
    case EbtInt:                mangledName += 'i';      break;
    case EbtUint:               mangledName += 'u';      break;
    case EbtBool:               mangledName += 'b';      break;
    case EbtSampler:
        switch (sampler.type) {
        case EbtInt:   mangledName += "i"; break;
        case EbtUint:  mangledName += "u"; break;
        default: break; // some compilers want this
        }
        if (sampler.image)
            mangledName += "I";
        else
            mangledName += "s";
        if (sampler.arrayed)
            mangledName += "A";
        if (sampler.shadow)
            mangledName += "S";
        switch (sampler.dim) {
        case Esd1D:       mangledName += "1";  break;
        case Esd2D:       mangledName += "2";  break;
        case Esd3D:       mangledName += "3";  break;
        case EsdCube:     mangledName += "C";  break;
        case EsdRect:     mangledName += "R2"; break;
        case EsdBuffer:   mangledName += "B";  break;
        default: break; // some compilers want this
        }
        break;
    case EbtStruct:
        mangledName += "struct-";
        if (typeName)
            mangledName += *typeName;
        for (unsigned int i = 0; i < structure->size(); ++i) {
            mangledName += '-';
            (*structure)[i].type->buildMangledName(mangledName);
        }
    default:
        break;
    }

    if (getVectorSize() > 0)
        mangledName += static_cast<char>('0' + getVectorSize());
    else {
        mangledName += static_cast<char>('0' + getMatrixCols());
        mangledName += static_cast<char>('0' + getMatrixRows());
    }

    if (arraySizes) {
        const int maxSize = 11;
        char buf[maxSize];
        snprintf(buf, maxSize, "%d", arraySizes->sizes.front());
        mangledName += '[';
        mangledName += buf;
        mangledName += ']';
    }
}

int TType::getStructSize() const
{
    if (!getStruct()) {
        assert(false && "Not a struct");
        return 0;
    }

    if (structureSize == 0)
        for (TTypeList::iterator tl = getStruct()->begin(); tl != getStruct()->end(); tl++)
            structureSize += ((*tl).type)->getObjectSize();

    return structureSize;
}

//
// Dump functions.
//

void TVariable::dump(TInfoSink& infoSink) const
{
    infoSink.debug << getName().c_str() << ": " << type.getStorageQualifierString() << " " << type.getBasicTypeString();
    if (type.isArray()) {
        infoSink.debug << "[0]";
    }
    infoSink.debug << "\n";
}

void TFunction::dump(TInfoSink& infoSink) const
{
    infoSink.debug << getName().c_str() << ": " <<  returnType.getBasicTypeString() << " " << getMangledName().c_str() << "\n";
}

void TAnonMember::dump(TInfoSink& TInfoSink) const
{
    TInfoSink.debug << "anonymous member " << getMemberNumber() << " of " << getAnonContainer().getName().c_str() << "\n";
}

void TSymbolTableLevel::dump(TInfoSink &infoSink) const
{
    tLevel::const_iterator it;
    for (it = level.begin(); it != level.end(); ++it)
        (*it).second->dump(infoSink);
}

void TSymbolTable::dump(TInfoSink &infoSink) const
{
    for (int level = currentLevel(); level >= 0; --level) {
        infoSink.debug << "LEVEL " << level << "\n";
        table[level]->dump(infoSink);
    }
}

//
// Functions have buried pointers to delete.
//
TFunction::~TFunction()
{
    for (TParamList::iterator i = parameters.begin(); i != parameters.end(); ++i)
        delete (*i).type;
}

//
// Symbol table levels are a map of pointers to symbols that have to be deleted.
//
TSymbolTableLevel::~TSymbolTableLevel()
{
    for (tLevel::iterator it = level.begin(); it != level.end(); ++it)
        delete (*it).second;

    delete [] defaultPrecision;
}

//
// Change all function entries in the table with the non-mangled name
// to be related to the provided built-in operation.
//
void TSymbolTableLevel::relateToOperator(const char* name, TOperator op)
{
    tLevel::const_iterator candidate = level.lower_bound(name);
    while (candidate != level.end()) {
        const TString& candidateName = (*candidate).first;
        TString::size_type parenAt = candidateName.find_first_of('(');
        if (parenAt != candidateName.npos && candidateName.compare(0, parenAt, name) == 0) {
            TFunction* function = (*candidate).second->getAsFunction();
            function->relateToOperator(op);
        } else
            break;
        ++candidate;
    }
}

// Make all function overloads of the given name require an extension(s).
// Should only be used for a version/profile that actually needs the extension(s).
void TSymbolTableLevel::setFunctionExtensions(const char* name, int num, const char* const extensions[])
{
    tLevel::const_iterator candidate = level.lower_bound(name);
    while (candidate != level.end()) {
        const TString& candidateName = (*candidate).first;
        TString::size_type parenAt = candidateName.find_first_of('(');
        if (parenAt != candidateName.npos && candidateName.compare(0, parenAt, name) == 0) {
            TSymbol* symbol = candidate->second;
            symbol->setExtensions(num, extensions);
        } else
            break;
        ++candidate;
    }
}

//
// Make all symbols in this table level read only.
//
void TSymbolTableLevel::readOnly()
{
    for (tLevel::iterator it = level.begin(); it != level.end(); ++it)
        (*it).second->makeReadOnly();
}

//
// Copy a symbol, but the copy is writable; call readOnly() afterward if that's not desired.
//
TSymbol::TSymbol(const TSymbol& copyOf)
{
    name = NewPoolTString(copyOf.name->c_str());
    uniqueId = copyOf.uniqueId;
    writable = true;
}

TVariable::TVariable(const TVariable& copyOf) : TSymbol(copyOf)
{	
    type.deepCopy(copyOf.type);
    userType = copyOf.userType;
    extensions = 0;
    setExtensions(copyOf.numExtensions, copyOf.extensions);

    if (! copyOf.unionArray.empty()) {
        assert(!copyOf.type.getStruct());
        assert(copyOf.type.getObjectSize() == 1);
        TConstUnionArray newArray(1);
        newArray[0] = copyOf.unionArray[0];
        unionArray = newArray;
    }
}

TVariable* TVariable::clone() const
{
    TVariable *variable = new TVariable(*this);

    return variable;
}

TFunction::TFunction(const TFunction& copyOf) : TSymbol(copyOf)
{	
    for (unsigned int i = 0; i < copyOf.parameters.size(); ++i) {
        TParameter param;
        parameters.push_back(param);
        parameters.back().copyParam(copyOf.parameters[i]);
    }

    extensions = 0;    
    setExtensions(copyOf.numExtensions, copyOf.extensions);
    returnType.deepCopy(copyOf.returnType);
    mangledName = copyOf.mangledName;
    op = copyOf.op;
    defined = copyOf.defined;
}

TFunction* TFunction::clone() const
{
    TFunction *function = new TFunction(*this);

    return function;
}

TAnonMember* TAnonMember::clone() const
{
    // Anonymous members of a given block should be cloned at a higher level,
    // where they can all be assured to still end up pointing to a single
    // copy of the original container.
    assert(0);

    return 0;
}

TSymbolTableLevel* TSymbolTableLevel::clone() const
{
    TSymbolTableLevel *symTableLevel = new TSymbolTableLevel();
    symTableLevel->anonId = anonId;
    std::vector<bool> containerCopied(anonId, false);
    tLevel::const_iterator iter;
    for (iter = level.begin(); iter != level.end(); ++iter) {
        const TAnonMember* anon = iter->second->getAsAnonMember();
        if (anon) {
            // Insert all the anonymous members of this same container at once,
            // avoid inserting the other members in the future, once this has been done,
            // allowing them to all be part of the same new container.
            if (! containerCopied[anon->getAnonId()]) {
                TVariable* container = anon->getAnonContainer().clone();
                container->changeName(NewPoolTString(""));
                // insert the whole container
                symTableLevel->insert(*container);
                containerCopied[anon->getAnonId()] = true;
            }
        } else
            symTableLevel->insert(*iter->second->clone());
    }

    return symTableLevel;
}

void TSymbolTable::copyTable(const TSymbolTable& copyOf)
{
    assert(adoptedLevels == copyOf.adoptedLevels);

    uniqueId = copyOf.uniqueId;
    noBuiltInRedeclarations = copyOf.noBuiltInRedeclarations;
    for (unsigned int i = copyOf.adoptedLevels; i < copyOf.table.size(); ++i)
        table.push_back(copyOf.table[i]->clone());
}

} // end namespace glslang
