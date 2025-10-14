/*
======================================
MIT License

Copyright (c) 2025 Adedoyin Aromolaran

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

======================================

Structure of Standard JSON:

(opening bracket)(whitespace, e.g space, tab, newline)(open quote mark)(array of chars)(close quote mark)(whitespace)(colon)(whitespace)(value: can be obj, string, array, or number)(close bracket)
  {                                                           "          hello world          "                          :                    "value"                                       }
*/

/*ArolanJSON.h: JSON Parser. Easily search through all elements.
Creates all the JSON member structs with malloc.
Delete the root AJObject with DeleteAJObject().
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//forward declarations
struct ArolanJSON;
struct AJObject;
struct AJString;
struct AJNumber;
struct AJArray;
struct AJArrayElement;
struct AJBoolean;
struct AJKeyValuePair;

#define TYPE_OBJECT 0
#define TYPE_STRING 1
#define TYPE_NUMBER 2
#define TYPE_ARRAY 3
#define TYPE_BOOLEAN 4
#define TYPE_NULL 5

//Reference to a collection of key value pairs
struct AJObject{
  struct AJKeyValuePair * FirstAJKVP; //if null but an AJObject instance exists, then its an empty object
  int AJKVPCount;
};

//basic building block of JSON
struct AJKeyValuePair{
  void * key;
  void * value;

  //type of value: is it object, array, string, number?
  int KeyType;
  int ValueType;

  //previous and next KVP's that are on same nest level as this KVP in their AJObject
  struct AJKeyValuePair * PrevAJKVP;
  struct AJKeyValuePair * NextAJKVP;

};

//simple JSON string. Didnt want to just want to use "string" bc that might cause naming conflicts with existing code
struct AJString{
  char * string;
  int length;
};

struct AJNumber{
  double number;
};

//JSON Array. Arrays can have multiple types within them so we have to handle that.
struct AJArray{
  struct AJArrayElement * FirstElement;
  struct AJArrayElement * MiddleElement; //used to find indexes faster - unimplemented
  struct AJArrayElement * LastElement; //used to find indexes faster - unimplemented
  int length;
};

  struct AJArrayElement{
    void * ArrayElement;
    int ArrayElementType;

    struct AJArrayElement * PrevAJElement;
    struct AJArrayElement * NextAJElement;

  };

struct AJBoolean{
  char TruthValue; // 0 abi 1
};

struct AJNull{
  char dummy; //in practice, just check for AJNull presence
  //NOTE: I wanted to make this an empty struct but empty struct behavior is inconsistent across compilers.
};

//internal functions and global variables
int __internal__DefaultStringLen = 10; //when increasing the size of a string
char __internal__DefaultDoublePrintDigitCount[] = "3"; //since ArolanJSON uses doubles internally for representing ALL numbers, we use this to format the printing
char finalNumberFormatString[5];//<-- increase size of this array when __internal__DefaultDoublePrintDigitCount goes beyond 2 digits
int __internal__DefaultReallocIncreaseSize = 200; //for writing AJ Types to buffer, how much do we realloc when buffer is full
#define CASE_INSENSITIVE 0
#define CASE_SENSITIVE 1
//creates the double printing format with a user - specified number of digits, and writes it to a buffer.
//params: finalFormatString (char array with length 4 minimum that the format string will be placed into)
//numberOfDigitsPastDecPoint (null terminated string of how many digits past decimal point preferred)
//useage: __internal__CreateNumberDigitFormat(string, "4");
void __internal__CreateNumberDigitFormat(char * finalFormatString, char * numberOfDigitsPastDecPoint){
  char dpPrefix[] = "%.";
  char dpSuffix[] = "f";

  finalFormatString[0] = '%';
  finalFormatString[1] = '.';
  char curr;
  int cabooseIndex = 0;
  int trainIndex = 2;
  while(1){
    curr = numberOfDigitsPastDecPoint[cabooseIndex];
    if(curr == '\0'){
      break;
    }
    finalFormatString[trainIndex] = curr;

    cabooseIndex++;
    trainIndex++;
  }

  finalFormatString[trainIndex++] = 'f';
  finalFormatString[trainIndex] = '\0';

}

int __internal__StringCompare(char * str1, char * str2, int caseSensitive){
  // Handle null pointer cases
  if(str1 == str2){ return 1;}  // Both same pointer (including both NULL)
  if(str1 == 0 || str2 == 0){ return 0;}  // One is NULL, other isn't

  int i = 0;

  while(str1[i] != '\0' && str2[i] != '\0'){
    char c1 = str1[i];
    char c2 = str2[i];

    if(caseSensitive){
      // Case sensitive comparison
      if(c1 != c2){
          return 0;
      }
    }else{
      // Case insensitive comparison
      // Convert to lowercase for comparison
      if(c1 >= 'A' && c1 <= 'Z'){
        c1 = c1 + ('a' - 'A');
      }
      if(c2 >= 'A' && c2 <= 'Z'){
        c2 = c2 + ('a' - 'A');
      }

      if(c1 != c2){
        return 0;
      }
    }
    i++;
  }

  // Check if both strings ended at the same time
  return (str1[i] == '\0' && str2[i] == '\0') ? 1 : 0;
}

int __internal__StringCopy(char * src, char * dest){
  int counter = 0;
  while(1){
    dest[counter] = src[counter];
    if(src[counter] == '\0'){break;}
    counter++;
  }
}

char * __internal__AddQuotesToString(char * unquotedString, char QuoteType){
  char * newString = (char*)malloc(sizeof(char) * __internal__DefaultStringLen);
  int newStringLen = __internal__DefaultStringLen;
  newString[0] = QuoteType;
  int idx = 1;
  char currentUnquotedStringChar;
  while(1){
    if(idx >= newStringLen - 1){
      newString = (char*)realloc(newString, newStringLen + __internal__DefaultStringLen);
      newStringLen+=__internal__DefaultStringLen;
    }
    currentUnquotedStringChar = unquotedString[idx-1];
    if(currentUnquotedStringChar == '\0'){
      //add closing quote and null terminator
      if(idx - newStringLen - 1 < 2){
        newString = (char*)realloc(newString, newStringLen + __internal__DefaultStringLen);
        newStringLen+=__internal__DefaultStringLen;
      }
      newString[idx++] = QuoteType;
      newString[idx] = '\0';
      //make as small as possible
      newString = (char*)realloc(newString, idx+1);
      return newString;
    }
    newString[idx] = currentUnquotedStringChar;
    idx++;
  }
}

//mallocs and copies a string, then returns
char * __internal__CreateAndCopy(char * src){
  char * ret = (char *)malloc(strlen(src) + 1);
  strcpy(ret, src);
  return ret;
}

//compare string but pointer logic is done for you.
int compareStringToAJString(char * inputstr, struct AJString * ajstr){
  return __internal__StringCompare(inputstr, ajstr->string, CASE_SENSITIVE);
}

//Primitive types are types that hold 1 value at a time; that is, not an array or object.
char isPrimitiveAJType(int type){
  if (type == TYPE_STRING || type == TYPE_NUMBER || type == TYPE_BOOLEAN || type == TYPE_NULL){return 1;}
  return 0;
}

//fast str comparison
static inline char compare3Chars(char * s1, char * s2){
  if(s1[0] == s2[0] && s1[1] == s2[1] && s1[2] == s2[2]){
    return 1;
  }else{
    return 0;
  }
}
static inline char compare4Chars(char * s1, char * s2){
  if(s1[0] == s2[0] && s1[1] == s2[1] && s1[2] == s2[2] && s1[3] == s2[3]){
    return 1;
  }else{
    return 0;
  }
}
static inline char compare5Chars(char * s1, char * s2){
  if(s1[0] == s2[0] && s1[1] == s2[1] && s1[2] == s2[2] && s1[3] == s2[3] && s1[4] == s2[4]){
    return 1;
  }else{
    return 0;
  }
}
static inline char compare6Chars(char * s1, char * s2){
  if(s1[0] == s2[0] && s1[1] == s2[1] && s1[2] == s2[2] && s1[3] == s2[3] && s1[4] == s2[4] && s1[5] == s2[5]){
    return 1;
  }else{
    return 0;
  }
}
static inline char compare7Chars(char * s1, char * s2){
  if(s1[0] == s2[0] && s1[1] == s2[1] && s1[2] == s2[2] && s1[3] == s2[3] && s1[4] == s2[4] && s1[5] == s2[5] && s1[6] == s2[6]){
    return 1;
  }else{
    return 0;
  }
}
static inline char compare8Chars(char * s1, char * s2){
  if(s1[0] == s2[0] && s1[1] == s2[1] && s1[2] == s2[2] && s1[3] == s2[3] && s1[4] == s2[4] && s1[5] == s2[5] && s1[6] == s2[6] && s1[7] == s2[7]){
    return 1;
  }else{
    return 0;
  }
}
static inline char compare9Chars(char * s1, char * s2){
  if(s1[0] == s2[0] && s1[1] == s2[1] && s1[2] == s2[2] && s1[3] == s2[3] && s1[4] == s2[4] && s1[5] == s2[5] && s1[6] == s2[6] && s1[7] == s2[7] && s1[8] == s2[8]){
    return 1;
  }else{
    return 0;
  }
}
static inline char compare10Chars(char * s1, char * s2){
  if(s1[0] == s2[0] && s1[1] == s2[1] && s1[2] == s2[2] && s1[3] == s2[3] && s1[4] == s2[4] && s1[5] == s2[5] && s1[6] == s2[6] && s1[7] == s2[7] && s1[8] == s2[8] && s1[9] == s2[9]){
    return 1;
  }else{
    return 0;
  }
}

//JSON parse type identifiers
#define openObjectBracket '{'
#define openArrayBracket '['
#define closeObjectBracket '}'
#define closeArrayBracket ']'
#define quoteMark_1 '"'
#define quoteMark_2 '\''
#define truthValueLetter_t 't'
#define truthValueLetter_f 'f'
#define nullValueLetter_n 'n'
#define escape '\\'
#define colon ':'
#define comma ','
#define indentation ' ' //<-- for pretty print

struct AJString * ParseNewAJString(int indexOfOpeningQuoteMark, char * JSONString, int * returnIdx);
struct AJNumber * ParseNewAJNumber(int indexOfFirstDigit, char * JSONString, int * returnIdx);
struct AJArray * ParseNewAJArray(int indexOfOpeningArrayBracket, char * JSONString, int * returnIdx);
struct AJObject * ParseNewAJObject(int indexOfOpeneingBracket, char * JSONString, int * returnIdx);
struct AJBoolean * ParseNewAJBoolean(int indexOfLetterTOrF, char * JSONString, int * returnIdx);
struct AJNull * ParseNewAJNull(int indexOfLetterN, char * JSONString, int * returnIdx);
void PrettyPrintKVP(struct AJKeyValuePair * kvp, int indentationCount);
void PrettyPrintAJArray(struct AJArray * aja, int indentationCount);
void PrettyPrintAJObject(struct AJObject * ajo, int indentationCount);
void DeleteAJArray(struct AJArray * aja);
void DeleteAJObject(struct AJObject * ajo);
void * GetElementFromObject(int startIndex, void * startKVP, int destIndex, int iterableType);
struct AJArrayElement * GetElementFromArrayIndex(struct AJArrayElement * startElement, int startIndex,  int destIndex);
struct AJKeyValuePair * GetKVPFromObjectIndex(struct AJKeyValuePair * startKVP, int startIndex,  int destIndex);
void RemoveFromAJArray(struct AJArray * ajarr, int idx);
void AddToAJArray(struct AJArray * ajarr, void * JSONElement, int elementType, int idx);
void AJDelete(void * aj, int elementType);

struct AJObject * CreateAJObject();
struct AJString * CreateAJString(char * string);
struct AJNumber * CreateAJNumber(float num);
struct AJArray * CreateAJArray();
struct AJBoolean * CreateAJBoolean();
struct AJNull * CreateAJNull();
struct AJKeyValuePair * CreateAJKeyValuePair(void * objectKey, int KeyType, void * objectValue, int ValueType);

void AddToAJObject(struct AJObject * ajo, struct AJKeyValuePair * newAJKVP, int position);
void RemoveFromAJObject(void * key, int KeyType, void * value, int ValueType);
void RemoveFromAJArray(struct AJArray * ajarr, int idx);
void AddToAJArray(struct AJArray * ajarr, void * JSONElement, int elementType, int idx);

int WriteAJArrayAsStringToBuffer(struct AJArray * aja, char ** originalBufferPointer, int * buflength, int positionToStartWriting);
int WriteAJObjectAsStringToBuffer(struct AJObject * ajo, char ** originalBufferPointer, int * buflength, int positionToStartWriting);

struct AJObject * CreateAJObject(){
  struct AJObject * adedoyin = (struct AJObject *)malloc(sizeof(struct AJObject));
  adedoyin->AJKVPCount = 0;
  adedoyin->FirstAJKVP = NULL;
  return adedoyin;
}

struct AJString * CreateAJString(char * string){
  struct AJString * pelumi = (struct AJString *)malloc(sizeof(struct AJString));
  pelumi->string = __internal__CreateAndCopy(string);
  pelumi->length = __internal__DefaultStringLen;
  return pelumi;
}

struct AJNumber * CreateAJNumber(float num){
  struct AJNumber * ayomide = (struct AJNumber *)malloc(sizeof(struct AJNumber));
  ayomide->number = num;
  return ayomide;
}

struct AJArray * CreateAJArray(){
  struct AJArray * opeyemi = (struct AJArray *)malloc(sizeof(struct AJArray));
  opeyemi->length = 0;
  opeyemi->FirstElement = NULL;
  return opeyemi;
}

struct AJBoolean * CreateAJBoolean(char * val){
  struct AJBoolean * femi = (struct AJBoolean *)malloc(sizeof(struct AJBoolean));
  femi->TruthValue = 0;
  if(val[0] == 't' || val[0] == 'T'){femi->TruthValue = 1;}
  return femi;
}

struct AJNull * CreateAJNull(){
  return (struct AJNull *)malloc(sizeof(struct AJNull));
}

struct AJKeyValuePair * CreateAJKeyValuePair(void * objectKey, int KeyType, void * objectValue, int ValueType){
  struct AJKeyValuePair * joju = (struct AJKeyValuePair *)malloc(sizeof(struct AJKeyValuePair));
  joju->PrevAJKVP = NULL;
  joju->NextAJKVP = NULL;
  joju->key = objectKey;
  joju->value = objectValue;
  joju->KeyType = KeyType;
  joju->ValueType = ValueType;

  return joju;
}

void AddToAJObject(struct AJObject * ajo, struct AJKeyValuePair * ajkvp, int position){
  ajkvp->NextAJKVP = NULL;
  ajkvp->PrevAJKVP = NULL;
  //check to make sure we arent appending (position == ajo->AJKVPCount)
  if(position > ajo->AJKVPCount || position < 0){
    return;
  }

  if(ajo->AJKVPCount == 0){
    ajo->FirstAJKVP = ajkvp;
    ajo->AJKVPCount++;
    return;
  }

  struct AJKeyValuePair * adjacentKVP;
  if(position == ajo->AJKVPCount){//append to end
    adjacentKVP = GetKVPFromObjectIndex(ajo->FirstAJKVP, 0, position-1);
    adjacentKVP->NextAJKVP = ajkvp;
    ajkvp->PrevAJKVP = adjacentKVP;
    ajkvp->NextAJKVP = NULL;
    ajo->AJKVPCount++;
  }else{
    //'put - behind' approach
    adjacentKVP = GetKVPFromObjectIndex(ajo->FirstAJKVP, 0, position);
    struct AJKeyValuePair * beforeThis = adjacentKVP->PrevAJKVP;
    if(beforeThis != NULL){
      beforeThis->NextAJKVP = ajkvp;
      ajkvp->PrevAJKVP = beforeThis;
    }
    ajkvp->NextAJKVP = adjacentKVP;
    adjacentKVP->PrevAJKVP = ajkvp;
  }
}

//remove ajelement at specified index
void RemoveFromAJArray(struct AJArray * ajarr, int idx){
  struct AJArrayElement * el = GetElementFromArrayIndex(ajarr->FirstElement, 0,idx);
  if(el == NULL){return;}
  //link prev elem to next as long as both are not null. in the case that either are null, do nothing for the one that is null.
  struct AJArrayElement * prevToEl = el->PrevAJElement;
  struct AJArrayElement * nextToEl = el->NextAJElement;

  if(prevToEl != NULL){
    prevToEl->NextAJElement = nextToEl;
  }
  if(nextToEl != NULL){
    nextToEl->PrevAJElement = prevToEl;
  }

  AJDelete(el, el->ArrayElementType);

  ajarr->length--;
}

//add an element to the ajarr. idx means 'i want to make this element the new idxth element'.
void AddToAJArray(struct AJArray * ajarr, void * JSONElement, int elementType, int idx){
  if(JSONElement == NULL || idx < 0 || idx > ajarr->length){return;}
  struct AJArrayElement * el = (struct AJArrayElement*)malloc(sizeof(struct AJArrayElement));
  el->ArrayElement = JSONElement;
  el->ArrayElementType = elementType;
  el->PrevAJElement = NULL;
  el->NextAJElement = NULL;

  if(ajarr->length == 0){
    //just add as first element
    ajarr->FirstElement = el;
    ajarr->length++;
    return;
  }

  struct AJArrayElement * currElementAtThisIndex;
  if(idx == ajarr->length){//adding to the end
    //put it after currElementAtThisIndex
    currElementAtThisIndex = GetElementFromArrayIndex(ajarr->FirstElement, 0, idx-1);
    el->PrevAJElement = currElementAtThisIndex;
    currElementAtThisIndex->NextAJElement = el;
  }else{
    //otherwise, use a 'put - behind' approach
    currElementAtThisIndex = GetElementFromArrayIndex(ajarr->FirstElement, 0, idx);
    struct AJArrayElement * pr = currElementAtThisIndex->PrevAJElement;
    if(pr != NULL){
      pr->NextAJElement = el;
      el->PrevAJElement = pr;
    }
    currElementAtThisIndex->PrevAJElement = el;
    el->NextAJElement = currElementAtThisIndex;
  }
  ajarr->length++;

}

/*ParseNewAJString takes a char array and an Index to where you encountered the first quoteMark_1 or quoteMark_2.
It reads forward -saving all the chars into the new AJString struct - until it finds the same quote mark
again (unescaped). It returns the index where it stopped (i.e where the closing quote mark is).
All AJStrings are null - terminated. All AJStrings contain the two quote marks that enclose it.*/
struct AJString * ParseNewAJString(int indexOfOpeningQuoteMark, char * JSONString, int * returnIdx){
  //create and init new struct
  struct AJString * pelumi = (struct AJString *)malloc(sizeof(struct AJString));
  pelumi->string = (char *)malloc(sizeof(char) * __internal__DefaultStringLen); //will resize as more chars are read.
  pelumi->length = __internal__DefaultStringLen;


  //read all chars starting with the opening quote.
      //(determine which quote type it is)
      char QuoteType = JSONString[indexOfOpeningQuoteMark];
      if(QuoteType != quoteMark_1 && QuoteType != quoteMark_2){
        //write to the error buffer and return null
        free(pelumi->string);
        free(pelumi);
        return NULL;
      }

  //start reading here
  // pelumi->string[0] = QuoteType;
  // int charsWritten = 1;
  int charsWritten = 0;
  int JSONCharIndex = indexOfOpeningQuoteMark + 1; //first char after quote
  char currentChar;
  int isEscaped = 0;
  int atClosingQuoteMark = 0;

  while(1){
    currentChar = JSONString[JSONCharIndex];
    // printf("current char: %d(%c) | position: %d\n", currentChar, currentChar , JSONCharIndex);

    if(currentChar == QuoteType && JSONString[JSONCharIndex-1] != escape){//we may be ending the string. check to see if escaped
      //write final quote and null ending to pelumi->string
      atClosingQuoteMark = 1;
      goto EndStringParse;
    }

    pelumi->string[charsWritten] = currentChar;
    charsWritten++;

    if(pelumi->length <= charsWritten){//resize by __internal__DefaultStringLen
      pelumi->string = (char*)realloc(pelumi->string, sizeof(char) * (pelumi->length + __internal__DefaultStringLen));
      pelumi->length += __internal__DefaultStringLen;
    }

    EndStringParse:
    if(atClosingQuoteMark == 1){
      pelumi->string[charsWritten++] = '\0';
      break;
    }

    JSONCharIndex++;
  }

  //realloc again to make sure string is exactly as long as it needs to be
  if(pelumi->length > charsWritten){
    pelumi->string = (char*)realloc(pelumi->string, sizeof(char) * charsWritten);
    pelumi->length = charsWritten;
  }
  *returnIdx = JSONCharIndex;
  return pelumi;
}

//takes in a pointer to the first digit of a number (we define 'digit' as any char 0-9 that isnt part of a string).
struct AJNumber * ParseNewAJNumber(int indexOfFirstDigit, char * JSONString, int * returnIdx){
  struct AJNumber * ayomide = (struct AJNumber *)malloc(sizeof(struct AJNumber));

  struct numHolder{
    int digit; //0-9
    struct numHolder * next;
  };
  char numParts[11] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.'};

  int digitsRead = 0;
  char currentChar;

  int positionOfDecPoint = -1;
  struct numHolder * first = (struct numHolder *)malloc(sizeof(struct numHolder));
  first->next = NULL;
  struct numHolder * current = first;
  int firstDigitHandled = 0;
  struct numHolder * previous;
  while(1){ //stop when a char is reached that isnt a digit or decimal point.
    currentChar = JSONString[digitsRead + indexOfFirstDigit];
    // printf("%c\n", currentChar );
    //check if the currentChar is part of a/ther number
    int isPart = 0;
    for(int i = 0; i < 11; i++){
      if(currentChar == numParts[i]){
        isPart = 1;
        break;
      }
    }
    if(isPart == 1){
      //check if . or digit
      if(currentChar == '.'){
        positionOfDecPoint = digitsRead;
      }else{ //must be a digit
        if(firstDigitHandled == 0){
          current->digit = currentChar - 48;
          firstDigitHandled = 1;
          previous = current;
        }else{
          struct numHolder * neue = (struct numHolder *)malloc(sizeof(struct numHolder));
          neue->digit = currentChar - 48;//digits are asci codes 48-57. Subtract the char value from 48 to get the digit.
          neue->next = NULL; //An embarrasing amount of time was spent figuring out that i needed to add this.
          current->next = neue;
          previous = current;
          current = neue;
        }

      }
      digitsRead++;
    }else{
      break;
    }
  }

  //read through the numholder linked list. if positionOfDecPoint > -1 then 10^x starts at x = positionOfDecPoint-1.
  //read from pos 0 till positionOfDecPoint, subtracting 1 for every exponent.
  current = first;
  double finalNumber = 0;
  int exponent = positionOfDecPoint != -1 ? positionOfDecPoint-1 : digitsRead-1;
  int c = 0;
  while(1){
    // printf("second loop");
    //get 10^x value
    double TenToTheExponentValue = 1;
    if(exponent >-1){
      for(int i = 0; i < exponent; i++){
        TenToTheExponentValue = TenToTheExponentValue * 10;
      }
    }else{//divide
      for(int i = 0; i > exponent; i--){
        TenToTheExponentValue = TenToTheExponentValue / 10;
      }
    }
    exponent--;
    finalNumber+= current->digit * TenToTheExponentValue;
    // printf("final num: %lf | exponent: %d | TenToTheExponentValue: %lf\n", finalNumber, exponent, TenToTheExponentValue);

    //delete element in linked list. Aint doing a mem leaks ting blud. ya get me fam.
    if(current->next == NULL){//end of digit seq
      free(current);
      break;
    }
    struct numHolder * hold = current;
    current = current->next;
    free(hold);


  }

  digitsRead--;//the logic of incrementing digitsRead makes it stop on the char directly AFTER the last char.
  //to maintain consistency across Parse functions, decrement one to represent the index with the last digit
  *returnIdx+=digitsRead;
  ayomide->number = finalNumber;
  return ayomide;
}

struct AJArray * ParseNewAJArray(int indexOfOpeningArrayBracket, char * JSONString, int * returnIdx){
  // create and init new struct
  struct AJArray * opeyemi = (struct AJArray *)malloc(sizeof(struct AJArray));
  opeyemi->length = 0;
  struct AJArrayElement * currentArrayElement;
  struct AJArrayElement * previousArrayElement = NULL;
  int JSONCharIndex = indexOfOpeningArrayBracket+1; //skip over opening array bracket character
  char currentChar = '\0';
  char reachedCloseBracket = 0;
  char skipThisChar = 0;
  //loop over all data in the array. stop when encounter the closing array character
  while(1){
    currentChar = JSONString[JSONCharIndex];

    //NOTE: I mallocd the currentArrayElement repeatedly in the cases because that is the only time we are guaranteed to have a new object.
    //didnt want to malloc > see its at the end > then free again
    switch(currentChar){
      case quoteMark_1:{
        struct AJString * element = ParseNewAJString(JSONCharIndex, JSONString, &JSONCharIndex);
        currentArrayElement = (struct AJArrayElement *)malloc(sizeof(struct AJArrayElement));
        currentArrayElement->ArrayElement = (void *)element;
        currentArrayElement->ArrayElementType = TYPE_STRING;
        break;
      }
      case quoteMark_2:{
        struct AJString * element = ParseNewAJString(JSONCharIndex, JSONString, &JSONCharIndex);
        currentArrayElement = (struct AJArrayElement *)malloc(sizeof(struct AJArrayElement));
        currentArrayElement->ArrayElement = (void *)element;
        currentArrayElement->ArrayElementType = TYPE_STRING;
        break;
      }
      case truthValueLetter_t:{
        struct AJBoolean * element = ParseNewAJBoolean(JSONCharIndex, JSONString, &JSONCharIndex);
        currentArrayElement = (struct AJArrayElement *)(malloc(sizeof(struct AJArrayElement)));
        currentArrayElement->ArrayElement = (void *)element;
        currentArrayElement->ArrayElementType = TYPE_BOOLEAN;
        break;
      }
      case truthValueLetter_f:{
        struct AJBoolean * element = ParseNewAJBoolean(JSONCharIndex, JSONString, &JSONCharIndex);
        currentArrayElement = (struct AJArrayElement *)malloc(sizeof(struct AJArrayElement));
        currentArrayElement->ArrayElement = (void *)element;
        currentArrayElement->ArrayElementType = TYPE_BOOLEAN;
        break;
      }
      case nullValueLetter_n:{
        struct AJNull * element = ParseNewAJNull(JSONCharIndex, JSONString, &JSONCharIndex);
        currentArrayElement = (struct AJArrayElement *)malloc(sizeof(struct AJArrayElement));
        currentArrayElement->ArrayElement = (void *)element;
        currentArrayElement->ArrayElementType = TYPE_NULL;
        break;
      }
      case openArrayBracket:{
        struct AJArray * element = ParseNewAJArray(JSONCharIndex, JSONString, &JSONCharIndex);
        currentArrayElement = (struct AJArrayElement *)malloc(sizeof(struct AJArrayElement));
        currentArrayElement->ArrayElement = (void *)element;
        currentArrayElement->ArrayElementType = TYPE_ARRAY;
        break;
      }
      case openObjectBracket:{
        struct AJObject * element = ParseNewAJObject(JSONCharIndex, JSONString, &JSONCharIndex);
        currentArrayElement = (struct AJArrayElement *)malloc(sizeof(struct AJArrayElement));
        currentArrayElement->ArrayElement = (void *)element;
        currentArrayElement->ArrayElementType = TYPE_OBJECT;
        break;
      }
      case closeArrayBracket:{
        reachedCloseBracket = 1;
        // printf("done!\n");
        break;
      }

      default:{
        //check if it is a number
        char numParts[11] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.'};
        char isNumber = 0;
        for(int i = 0; i < 11; i++){
          if(currentChar == numParts[i]){
            isNumber = 1;
            break;
          }
        }
        if(isNumber == 1){
          struct AJNumber * element = ParseNewAJNumber(JSONCharIndex, JSONString, &JSONCharIndex);
          currentArrayElement = (struct AJArrayElement *)malloc(sizeof(struct AJArrayElement));
          currentArrayElement->ArrayElement = (void *)element;
          currentArrayElement->ArrayElementType = TYPE_NUMBER;
          break;
        }else{
          // printf("skipping over char '%c' at index %d\n", currentChar, JSONCharIndex);
          skipThisChar = 1;
        }
      }
    }

    if(reachedCloseBracket == 1){
      break;
    }

    if(skipThisChar == 0){
      //after creating new element, add to array
      currentArrayElement->PrevAJElement = previousArrayElement; //link current to previous element
      currentArrayElement->NextAJElement = NULL;
      if(previousArrayElement != NULL){
        previousArrayElement->NextAJElement = currentArrayElement; //link previous to next (null check b/c on first iteration prev is null.)
      }
      previousArrayElement = currentArrayElement; //current element will become previous
      // currentArrayElement = (struct AJArrayElement *)malloc(sizeof(struct AJArrayElement)); //malloc the next element onto heap
      if(opeyemi->length == 0){//this is the first element
        opeyemi->FirstElement = currentArrayElement;
      }
      opeyemi->length++;
    }else{
      skipThisChar = 0; //set back to 0
    }
    JSONCharIndex++;

  }

  *returnIdx = JSONCharIndex;
  if(opeyemi->length == 0){//it was empty; dealloc the OG element we mallocd
    opeyemi->FirstElement = NULL;
  }
  return opeyemi;

}

struct AJObject * ParseNewAJObject(int indexOfOpeneingBracket, char * JSONString, int * returnIdx){
  struct AJObject * adedoyin = (struct AJObject *)malloc(sizeof(struct AJObject));
  adedoyin->AJKVPCount = 0;

  struct AJKeyValuePair * currentKVP = NULL; //current KVP having data put into it
  adedoyin->FirstAJKVP = NULL;
  struct AJKeyValuePair * previousKVP = NULL;
  int JSONCharIndex = indexOfOpeneingBracket + 1; //skip over opening pracket
  char currentChar;
  int skipCharacter = 0;
  int objectFinished = 0;
  int IS_KEY = 0;
  int IS_VALUE = 1;
  int currentKVPState = IS_KEY;
  int linkKVPsNow = 0;

  void * element;
  int elementType;

  while(1){
    currentChar = JSONString[JSONCharIndex];
    // printf("current char: %c | position: %d\n", currentChar , JSONCharIndex);
    switch(currentChar){
      case quoteMark_1:{
        element = (void*)ParseNewAJString(JSONCharIndex, JSONString, &JSONCharIndex);
        elementType = TYPE_STRING;
        break;
      }
      case quoteMark_2:{
        element = (void*)ParseNewAJString(JSONCharIndex, JSONString, &JSONCharIndex);
        elementType = TYPE_STRING;
        break;
      }
      case truthValueLetter_t:{
        element = (void*)ParseNewAJBoolean(JSONCharIndex, JSONString, &JSONCharIndex);
        elementType = TYPE_BOOLEAN;
        break;
      }
      case truthValueLetter_f:{
        element = (void*)ParseNewAJBoolean(JSONCharIndex, JSONString, &JSONCharIndex);
        elementType = TYPE_BOOLEAN;
        break;
      }
      case nullValueLetter_n:{
        element = (void*)ParseNewAJNull(JSONCharIndex, JSONString, &JSONCharIndex);
        elementType = TYPE_NULL;
        break;
      }
      case openObjectBracket:{
        element = (void*)ParseNewAJObject(JSONCharIndex, JSONString, &JSONCharIndex);
        elementType = TYPE_OBJECT;
        break;
      }
      case openArrayBracket:{
        element = (void*)ParseNewAJArray(JSONCharIndex, JSONString, &JSONCharIndex);
        elementType = TYPE_ARRAY;
        break;
      }
      case closeObjectBracket:{
        objectFinished = 1;
        linkKVPsNow = 1;
        break;
      }
      case colon:{ //switch from key to value
        currentKVPState = IS_VALUE;
        skipCharacter = 1;
        break;
      }
      case comma:{ //switch from value to key, also link current kvp's
        currentKVPState = IS_KEY;
        skipCharacter = 1;
        linkKVPsNow = 1;
        break;
      }
      default:{
        //check if number
        char numParts[11] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.'};
        char isNumber = 0;
        for(int i = 0; i < 11; i++){
          if(currentChar == numParts[i]){
            isNumber = 1;
            break;
          }
        }
        if(isNumber == 1){
          element = (void*)ParseNewAJNumber(JSONCharIndex, JSONString, &JSONCharIndex);
          elementType = TYPE_NUMBER;
        }else{
          skipCharacter = 1;
        }
        break;
      }

    }

    if(linkKVPsNow == 1){
      if(currentKVP != NULL){//was an empty KVP
        currentKVP->NextAJKVP = NULL;
        currentKVP->PrevAJKVP = previousKVP;
        if(previousKVP != NULL){
          previousKVP->NextAJKVP = currentKVP;
        }
        if(adedoyin->AJKVPCount == 0){
          adedoyin->FirstAJKVP = currentKVP;
        }
        previousKVP = currentKVP;
        adedoyin->AJKVPCount++;
        currentKVP = NULL;
        linkKVPsNow = 0;
      }else{//was an empty kvp
        //do nothing, this is just for info / readability on the thought process
      }

    }

    if(objectFinished == 1){
      break;
    }

    if(skipCharacter == 0){
      if(currentKVPState == IS_KEY){
        currentKVP = (struct AJKeyValuePair*)malloc(sizeof(struct AJKeyValuePair));
        currentKVP->key = element;
        currentKVP->KeyType = elementType;
      }else if(currentKVPState == IS_VALUE){
        currentKVP->value = element;
        currentKVP->ValueType = elementType;
        // if(elementType == TYPE_STRING){
        //   printf("%s\n", ((struct AJString*)element)->string);
        // }
      }
    }else{
      skipCharacter = 0;
    }

    JSONCharIndex++;

  }

  *returnIdx = JSONCharIndex;
  return adedoyin;
}

struct AJBoolean * ParseNewAJBoolean(int indexOfLetterTOrF, char * JSONString, int * returnIdx){
  // AJBoolean can ony represent true or false, so we only need to check the first letter (t or f)
  char TVal = -1;
  int charsToSkip = 0;
  if(JSONString[indexOfLetterTOrF] == 't'){
    TVal = (char)1;
    charsToSkip = 3; //3 more chars in word 'true' (after starting letter)
  }else if(JSONString[indexOfLetterTOrF] == 'f'){
    TVal = (char)0;
    charsToSkip = 4; //4 more chars in word 'false' (after starting letter)
  }else{
    return NULL;
  }
  struct AJBoolean * femi = (struct AJBoolean *)malloc(sizeof(struct AJBoolean));
  femi->TruthValue = TVal;
  *returnIdx = indexOfLetterTOrF + charsToSkip;
  return femi;
}

struct AJNull * ParseNewAJNull(int indexOfLetterN, char * JSONString, int * returnIdx){
  *returnIdx = indexOfLetterN + 3; //started at n, add 'ull'
  return (struct AJNull *)malloc(sizeof(struct AJNull));
}

//returns either AJKVP or AJArrayElement from Object or Array
void * GetElementFromObject(int startIndex, void * startKVP, int destIndex, int iterableType){

  void * current = startKVP;
  for(int i = startIndex; i < destIndex; i++){
    if(iterableType == TYPE_OBJECT){
      current = (void*)(((struct AJKeyValuePair*)current)->NextAJKVP);
    }else if(iterableType == TYPE_ARRAY){
      current = (void*)(((struct AJArrayElement*)current)->NextAJElement);
    }
    if(current == NULL){
      return NULL;
    }
  }
  return current;
}

struct AJArrayElement * GetElementFromArrayIndex(struct AJArrayElement * startElement, int startIndex,  int destIndex){
  if(startIndex > destIndex){return NULL;}
  struct AJArrayElement * current = startElement;
  while(startIndex != destIndex){
    current = current->NextAJElement;
    startIndex++;
  }
  return current;
}

struct AJKeyValuePair * GetKVPFromObjectIndex(struct AJKeyValuePair * startKVP, int startIndex,  int destIndex){
  if(startIndex > destIndex){return NULL;}
  struct AJKeyValuePair * current = startKVP;
  while(startIndex != destIndex){
    current = current->NextAJKVP;
    startIndex++;
  }
  return current;
}

void PrettyPrintKVP(struct AJKeyValuePair * kvp, int indentationCount){
  char whitespace[indentationCount+1];
  for(int i = 0; i < indentationCount; i++){
    whitespace[i] = indentation;
  }
  whitespace[indentationCount] = '\0';
  printf("%s", whitespace);
  //print key
  switch(kvp->KeyType){
    case TYPE_NUMBER :{
      double myNum = ((struct AJNumber*)(kvp->key))->number;
      __internal__CreateNumberDigitFormat(finalNumberFormatString, __internal__DefaultDoublePrintDigitCount);
      printf(finalNumberFormatString, myNum);
      break;
    }
    case TYPE_STRING :{
      printf("%s", ((struct AJString*)(kvp->key))->string);
      break;
    }

    case TYPE_BOOLEAN:{
      if(((struct AJBoolean*)(kvp->key))->TruthValue == 1){
        printf("true");
      }else{
        printf("false");
      }
      break;
    }

    case TYPE_NULL:{
      printf("null");
      break;
    }

    case TYPE_ARRAY :{
      PrettyPrintAJArray((struct AJArray*)(kvp->key), indentationCount);
      break;
    }

    case TYPE_OBJECT :{
      PrettyPrintAJObject((struct AJObject*)(kvp->key), indentationCount);
      break;
    }

  }

  printf(" : ");

  switch(kvp->ValueType){
    case TYPE_NUMBER :{
      double myNum = ((struct AJNumber*)(kvp->value))->number;
      __internal__CreateNumberDigitFormat(finalNumberFormatString, __internal__DefaultDoublePrintDigitCount);
      printf(finalNumberFormatString, myNum);
      break;
    }
    case TYPE_STRING :{
      printf("%s", ((struct AJString*)(kvp->value))->string);
      break;
    }

    case TYPE_BOOLEAN:{
      if(((struct AJBoolean*)(kvp->value))->TruthValue == 1){
        printf("true");
      }else{
        printf("false");
      }
      break;
    }

    case TYPE_NULL:{
      printf("null");
      break;
    }

    case TYPE_ARRAY :{
      PrettyPrintAJArray((struct AJArray*)(kvp->value), indentationCount);
      break;
    }

    case TYPE_OBJECT :{
      PrettyPrintAJObject((struct AJObject*)(kvp->value), indentationCount);
      break;
    }

  }
}

void PrettyPrintAJArray(struct AJArray * aja, int indentationCount){
  char whitespace[indentationCount+1];
  for(int i = 0; i < indentationCount; i++){
    whitespace[i] = indentation;
  }
  whitespace[indentationCount] = '\0';
  printf("%s[\n", whitespace);

  struct AJArrayElement * current = aja->FirstElement;
  int currentIndex = 0;
  for(int i = 0; i < aja->length; i++){
    // printf("%s%c", whitespace, indentation); //elements are 1 more indentation away from bracket
    switch (current->ArrayElementType) {
      case TYPE_NUMBER :{
        printf("%s%c", whitespace, indentation); //elements are 1 more indentation away from bracket
        double myNum = ((struct AJNumber*)(current->ArrayElement))->number;
        __internal__CreateNumberDigitFormat(finalNumberFormatString, __internal__DefaultDoublePrintDigitCount);
        printf(finalNumberFormatString, myNum);
        break;
      }
      case TYPE_STRING :{
        printf("%s%c", whitespace, indentation); //elements are 1 more indentation away from bracket
        printf("%s", ((struct AJString*)(current->ArrayElement))->string);
        break;
      }
      case TYPE_BOOLEAN:{
        if(((struct AJBoolean*)(current->ArrayElement))->TruthValue == 1){
          printf("%s%c", whitespace, indentation); //elements are 1 more indentation away from bracket
          printf("true");
        }else{
          printf("%s%c", whitespace, indentation); //elements are 1 more indentation away from bracket
          printf("false");
        }
        break;
      }
      case TYPE_NULL:{
        printf("%s%c", whitespace, indentation); //elements are 1 more indentation away from bracket
        printf("null");
        break;
      }
      case TYPE_ARRAY:{
        PrettyPrintAJArray((struct AJArray*)(current->ArrayElement), indentationCount+1);
        break;
      }
      case TYPE_OBJECT:{
        PrettyPrintAJObject((struct AJObject*)(current->ArrayElement), indentationCount+1);
        break;
      }
    }
    if(i != aja->length -1){
      printf(",");
    }
    printf("\n");
    current = current->NextAJElement;
  }
  printf("%s]", whitespace);
}

void PrettyPrintAJObject(struct AJObject * ajo, int indentationCount){
  char whitespace[indentationCount+1];
  for(int i = 0; i < indentationCount; i++){
    whitespace[i] = indentation;
  }
  whitespace[indentationCount] = '\0';
  printf("%s{\n", whitespace);

  struct AJKeyValuePair * current = ajo->FirstAJKVP;
  int currentIndex = 0;
  for(int i = 0; i < ajo->AJKVPCount; i++){
    printf("%c", indentation);
    PrettyPrintKVP(current, indentationCount);
    if(i != ajo->AJKVPCount -1){
      printf(",");
    }
    printf("\n");
    current = current->NextAJKVP;
  }

  printf("%s}", whitespace);
}

int WritePrimitiveTypeAsStringToBuffer(void* obj, int type, char ** originalBufferPointer, int * buflength, int positionToStartWriting){
    //writes a number, string, bool, or null to a HEAP CHAR ARRAY. the char array will be resized as necessary to fit everything.
    //it returns actual characters written count + 1 for the null terminator. so if sequentially writing to the same buffer,
    //make sure to DECREMENT the next positionToStartWriting arg to overwrite that null term.
    int digitsWritten = 0;
    (*originalBufferPointer)[positionToStartWriting] = '\0';

    switch(type){
        case TYPE_NUMBER :{
            // printf("Writing a number\n");
            double myNum = ((struct AJNumber*)obj)->number;
            char numStrBuf[326]; // chars needed to display the longest double string in C
            numStrBuf[0] = '\0';
            __internal__CreateNumberDigitFormat(finalNumberFormatString, __internal__DefaultDoublePrintDigitCount);
            digitsWritten = sprintf(numStrBuf, finalNumberFormatString, myNum) + 1; // include null ptr

            // Ensure buffer is large enough
            if(*buflength - positionToStartWriting < digitsWritten){
                *originalBufferPointer = realloc(*originalBufferPointer, *buflength + digitsWritten + __internal__DefaultReallocIncreaseSize);
                *buflength += digitsWritten + __internal__DefaultReallocIncreaseSize;
            }
            strcat(&(*originalBufferPointer)[positionToStartWriting], numStrBuf);
            break;
        }
        case TYPE_STRING :{
            // printf("Writing a string\n");
            char * s = ((struct AJString*)obj)->string;
            int slen = strlen(s);
            digitsWritten = slen + 3; // +2 for quotes, +1 for null terminator

            // Ensure buffer is large enough
            if(*buflength - positionToStartWriting < digitsWritten){
                *originalBufferPointer = realloc(*originalBufferPointer, *buflength + digitsWritten + __internal__DefaultReallocIncreaseSize);
                *buflength +=digitsWritten +  __internal__DefaultReallocIncreaseSize;
            }

            // Write opening quote
            (*originalBufferPointer)[positionToStartWriting] = '"';
            // Copy the string content
            strcpy(&(*originalBufferPointer)[positionToStartWriting + 1], s);
            // Write closing quote
            (*originalBufferPointer)[positionToStartWriting + 1 + slen] = '"';
            // Write null terminator
            (*originalBufferPointer)[positionToStartWriting + 1 + slen + 1] = '\0';
            break;
        }
        case TYPE_BOOLEAN:{
            // printf("Writing a bool\n");
            if(((struct AJBoolean*)obj)->TruthValue == 1){
                digitsWritten = 5;
                if(*buflength - positionToStartWriting < digitsWritten){
                    *originalBufferPointer = realloc(*originalBufferPointer, *buflength + __internal__DefaultReallocIncreaseSize);
                    *buflength += __internal__DefaultReallocIncreaseSize;
                }
                strcat(&(*originalBufferPointer)[positionToStartWriting], "true");
            }else{
                digitsWritten = 6;
                if(*buflength - positionToStartWriting < digitsWritten){
                    *originalBufferPointer = realloc(*originalBufferPointer, *buflength + __internal__DefaultReallocIncreaseSize);
                    *buflength += __internal__DefaultReallocIncreaseSize;
                }
                strcat(&(*originalBufferPointer)[positionToStartWriting], "false");
            }
            break;
        }
        case TYPE_NULL:{
            // printf("Writing a null value\n");
            digitsWritten = 5;
            if(*buflength - positionToStartWriting < digitsWritten){
                *originalBufferPointer = realloc(*originalBufferPointer, *buflength + __internal__DefaultReallocIncreaseSize);
                *buflength += __internal__DefaultReallocIncreaseSize;
            }
            strcat(&(*originalBufferPointer)[positionToStartWriting], "null");
            break;
        }
    }

    return digitsWritten;
}

int WriteAJArrayAsStringToBuffer(struct AJArray * aja, char ** originalBufferPointer, int * buflength, int positionToStartWriting){
    (*originalBufferPointer)[positionToStartWriting] = '\0';
    int bytesWritten = positionToStartWriting;

    //write "[ " to the buffer first
    if(*buflength - positionToStartWriting < 3){
        *originalBufferPointer = realloc(*originalBufferPointer, *buflength + __internal__DefaultReallocIncreaseSize);
        *buflength += __internal__DefaultReallocIncreaseSize;
    }
    strcat(&(*originalBufferPointer)[positionToStartWriting], "[ ");
    positionToStartWriting += 2; // "[ " is 2 chars (not counting null terminator)

    //loop over all the array elements
    struct AJArrayElement * current = aja->FirstElement;
    for(int i = 0; i < aja->length; i++){
        if(isPrimitiveAJType(current->ArrayElementType)){
            positionToStartWriting += WritePrimitiveTypeAsStringToBuffer(current->ArrayElement, current->ArrayElementType, originalBufferPointer, buflength, positionToStartWriting) - 1;
        }else{
            switch(current->ArrayElementType){
                case TYPE_ARRAY : {
                    positionToStartWriting += WriteAJArrayAsStringToBuffer(current->ArrayElement, originalBufferPointer, buflength, positionToStartWriting) - 1;
                    break;
                }
                case TYPE_OBJECT : {
                    positionToStartWriting += WriteAJObjectAsStringToBuffer(current->ArrayElement, originalBufferPointer, buflength, positionToStartWriting) - 1;
                    break;
                }
            }
        }
        if(i != aja->length - 1){
            if(*buflength - positionToStartWriting < 3){
                *originalBufferPointer = realloc(*originalBufferPointer, *buflength + __internal__DefaultReallocIncreaseSize);
                *buflength += __internal__DefaultReallocIncreaseSize;
            }
            strcat(&(*originalBufferPointer)[positionToStartWriting], ", ");
            positionToStartWriting += 2; // ", " is 2 chars
        }
        current = current->NextAJElement;
    }

    if(*buflength - positionToStartWriting < 3){
        *originalBufferPointer = realloc(*originalBufferPointer, *buflength + __internal__DefaultReallocIncreaseSize);
        *buflength += __internal__DefaultReallocIncreaseSize;
    }

    strcat(&(*originalBufferPointer)[positionToStartWriting], " ]");
    positionToStartWriting += 2; // " ]" is 2 chars

    return positionToStartWriting - bytesWritten + 1; // +1 for null terminator
}

int WriteAJObjectAsStringToBuffer(struct AJObject * ajo, char ** originalBufferPointer, int * buflength, int positionToStartWriting){
    (*originalBufferPointer)[positionToStartWriting] = '\0';
    int bytesWritten = positionToStartWriting;

    if(*buflength - positionToStartWriting < 3){
        *originalBufferPointer = realloc(*originalBufferPointer, *buflength + __internal__DefaultReallocIncreaseSize);
        *buflength += __internal__DefaultReallocIncreaseSize;
    }
    strcat(&(*originalBufferPointer)[positionToStartWriting], "{ ");
    positionToStartWriting += 2; // "{ " is 2 chars

    //loop over key value pairs
    struct AJKeyValuePair * ajkvp = ajo->FirstAJKVP;
    for(int i = 0; i < ajo->AJKVPCount; i++){
        // Write key
        if(isPrimitiveAJType(ajkvp->KeyType)){
            positionToStartWriting += WritePrimitiveTypeAsStringToBuffer(ajkvp->key, ajkvp->KeyType, originalBufferPointer, buflength, positionToStartWriting) - 1;
        }else{
            switch(ajkvp->KeyType){
                case TYPE_ARRAY : {
                    positionToStartWriting += WriteAJArrayAsStringToBuffer(ajkvp->key, originalBufferPointer, buflength, positionToStartWriting) - 1;
                    break;
                }
                case TYPE_OBJECT : {
                    positionToStartWriting += WriteAJObjectAsStringToBuffer(ajkvp->key, originalBufferPointer, buflength, positionToStartWriting) - 1;
                    break;
                }
            }
        }

        // Write separator
        if(*buflength - positionToStartWriting < 4){
            *originalBufferPointer = realloc(*originalBufferPointer, *buflength + __internal__DefaultReallocIncreaseSize);
            *buflength += __internal__DefaultReallocIncreaseSize;
        }
        strcat(&(*originalBufferPointer)[positionToStartWriting], " : ");
        positionToStartWriting += 3; // " : " is 3 chars

        // Write value
        if(isPrimitiveAJType(ajkvp->ValueType)){
            positionToStartWriting += WritePrimitiveTypeAsStringToBuffer(ajkvp->value, ajkvp->ValueType, originalBufferPointer, buflength, positionToStartWriting) - 1;
        }else{
            switch(ajkvp->ValueType){
                case TYPE_ARRAY : {
                    positionToStartWriting += WriteAJArrayAsStringToBuffer(ajkvp->value, originalBufferPointer, buflength, positionToStartWriting) - 1;
                    break;
                }
                case TYPE_OBJECT : {
                    positionToStartWriting += WriteAJObjectAsStringToBuffer(ajkvp->value, originalBufferPointer, buflength, positionToStartWriting) - 1;
                    break;
                }
            }
        }

        // Add comma if not last element
        if(i != ajo->AJKVPCount - 1){
            if(*buflength - positionToStartWriting < 3){
                *originalBufferPointer = realloc(*originalBufferPointer, *buflength + __internal__DefaultReallocIncreaseSize);
                *buflength += __internal__DefaultReallocIncreaseSize;
            }
            strcat(&(*originalBufferPointer)[positionToStartWriting], ", ");
            positionToStartWriting += 2; // ", " is 2 chars
        }

        ajkvp = ajkvp->NextAJKVP;
    }

    if(*buflength - positionToStartWriting < 3){
        *originalBufferPointer = realloc(*originalBufferPointer, *buflength + __internal__DefaultReallocIncreaseSize);
        *buflength += __internal__DefaultReallocIncreaseSize;
    }
    strcat(&(*originalBufferPointer)[positionToStartWriting], " }");
    positionToStartWriting += 2; // " }" is 2 chars

    return positionToStartWriting - bytesWritten + 1; // +1 for null terminator
}

void MinimizeCharArrayByteSize(char ** originalBufferPointer, int * len, int actualByteSize){
  *originalBufferPointer = realloc(*originalBufferPointer, actualByteSize);
  *len = actualByteSize;
}

int AppendToBuffer(char * toWrite, char ** originalBufferPointer, int * buflength, int positionToStartWriting){
  //write 'toWrite' to the end of originalBufferPointer (e.g *originalBufferPointer[positionToStartWriting]), and add a null char
  //resize buffer if necessary
  int lenToWrite = strlen(toWrite) + 1;
  if(*buflength < lenToWrite){
    *originalBufferPointer = realloc(*originalBufferPointer, *buflength + lenToWrite + __internal__DefaultReallocIncreaseSize);
    *buflength += lenToWrite + __internal__DefaultReallocIncreaseSize;
  }
  (*originalBufferPointer)[positionToStartWriting] = '\0';
  strcat(&(*originalBufferPointer)[positionToStartWriting], toWrite);
  return lenToWrite;
}

void PrintPureByteArray(char * toPrint, int length, char * howToPrint){
  if(howToPrint[0] == 'c'){
    for(int i = 0; i < length; i++){
      if(toPrint[i] == '\0'){
        printf("\\0");
      }else if(toPrint[i] == '\n'){
        printf("\\n\n");
      }else if(toPrint[i] == '\r'){
        printf("\\r");
      }else{
        printf("%c", toPrint[i]);
      }
    }
  }
  /*
  for(int i = 0; i < length; i++){
    if(toPrint[i] == '\0'){
      if(howToPrint[0] == 'c'){
        printf("\\0 ");
      }else{
        printf("0 ");
      }
    }else{
      if(howToPrint[0] == 'c'){
        printf("%c", toPrint[i]);
      }else{
        printf("%d ", toPrint[i]);
      }
    }
  }
  */
  printf("\n");
}

int AppendToBuffer_WithKnownLengthOfInput(char * toWrite, int lengthOfToWrite, char ** originalBufferPointer, int * buflength, int positionToStartWriting){
  //uses memcpy for known lengths (e.g if we copy a number, it may have 0x00 bytes and cause strcat to fail.)4
  if(*buflength < lengthOfToWrite + 1){
    *originalBufferPointer = realloc(*originalBufferPointer, *buflength + lengthOfToWrite + __internal__DefaultReallocIncreaseSize);
    *buflength += lengthOfToWrite + __internal__DefaultReallocIncreaseSize;
  }
  (*originalBufferPointer)[positionToStartWriting] = '\0';
  memcpy(&(*originalBufferPointer)[positionToStartWriting], toWrite, lengthOfToWrite);
  /*NOTE: Wasnt sure if we should add a null char at end if none is detected (prevent bugs) but
  i will because it messes up serial Appending logic (not knowing whether a null is already there or not).
  so by default, we will check if its ended by a null term byte, and add one if it isnt.
  remove / comment out this following code to prevent that.
  */
  positionToStartWriting+=lengthOfToWrite;
  (*originalBufferPointer)[positionToStartWriting+1] = '\0';
  return lengthOfToWrite+1;

}

char * LoadJSONFromFile(char * filename){
  FILE *file;
  long file_size;
  char *content;

  file = fopen(filename, "r");
  if(file == NULL) {
    printf("Error opening file\" %s \"", filename );
    return NULL;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Allocate memory (+1 for null terminator)
  content = malloc(file_size + 1);
  if(content == NULL) {
    printf("Memory allocation failed");
    fclose(file);
    return NULL;
  }

  // Read entire file
  fread(content, 1, file_size, file);
  content[file_size] = '\0';  // Null terminate

  fclose(file);
  return content;
}

//linear search for number or string
struct AJArrayElement * SearchArrayForElement(struct AJArray * arr, void * elem_NumOrString, int type, int* getIndexInArray){
  struct AJArrayElement * current = arr->FirstElement;
  int index = 0;
  if(type == TYPE_NUMBER){
    double searchFor = *((double*)elem_NumOrString);
    while(current != NULL){
      if(current->ArrayElementType == TYPE_NUMBER){
        if(((struct AJNumber*)(current->ArrayElement))->number == searchFor){
          *getIndexInArray = index;
          return current;
        }
      }
      index++;
      current = current->NextAJElement;
    }
  }else if(type == TYPE_STRING){
    char * str = (char *)elem_NumOrString;
    while(current != NULL){
      if(current->ArrayElementType == TYPE_STRING){
        if(compareStringToAJString(str, (struct AJString*)(current->ArrayElement)) == 1){
          *getIndexInArray = index;
          return current;
        }
      }
      index++;
      current = current->NextAJElement;
    }
  }
  return NULL;
}

struct AJKeyValuePair * SearchObjectForKey(char * key, struct AJObject * obj){
  if(key == NULL){return NULL;}


  struct AJKeyValuePair * current = obj->FirstAJKVP;
  while(current != NULL){
    if(current->KeyType == TYPE_STRING){
      if(compareStringToAJString(key, (struct AJString*)(current->key)) == 1){
        return current;
      }
    }
    current = current->NextAJKVP;
  }
  return NULL;
}

void DeleteAJArray(struct AJArray * aja){//linearly free all heap resources referenced by this AJA.
  struct AJArrayElement * AJae = aja->FirstElement;
  while(AJae != NULL){
    switch(AJae->ArrayElementType){
      case TYPE_NUMBER:
      case TYPE_BOOLEAN:
      case TYPE_NULL:{
        free(AJae->ArrayElement); //for these three, all memory is inside the struct
        break;
      }
      case TYPE_STRING:{
        struct AJString * hold = (struct AJString *)AJae->ArrayElement;
        free(hold->string);
        free(hold);
        break;
      }
      case TYPE_ARRAY:{
        struct AJArray * hold = (struct AJArray *)AJae->ArrayElement;
        DeleteAJArray(hold);
        break;
      }
      case TYPE_OBJECT:{
        struct AJObject * hold = (struct AJObject *)AJae->ArrayElement;
        DeleteAJObject(hold);
        break;
      }
    }
    struct AJArrayElement * prev = AJae;
    AJae = AJae->NextAJElement;
    free(prev);
  }
  free(aja);
}

void DeleteAJObject(struct AJObject * ajo){//recursively free all heap resources referenced by this AJO. Make sure to change references to this object when deleting.
  struct AJKeyValuePair * ak = ajo->FirstAJKVP;
  while (ak != NULL) {
    switch(ak->KeyType){
      case TYPE_NUMBER:
      case TYPE_BOOLEAN:
      case TYPE_NULL:{
        free(ak->key);
        break;
      }
      case TYPE_STRING:{
        struct AJString * hold = (struct AJString *)ak->key;
        free(hold->string);
        free(hold);
        break;
      }
      case TYPE_ARRAY:{
        struct AJArray * hold = (struct AJArray *)ak->key;
        DeleteAJArray(hold);
        break;
      }
      case TYPE_OBJECT:{
        struct AJObject * hold = (struct AJObject *)ak->key;
        DeleteAJObject(hold);
        break;
      }

    }

    switch(ak->ValueType){
      case TYPE_NUMBER:
      case TYPE_BOOLEAN:
      case TYPE_NULL:{
        free(ak->value);
        break;
      }
      case TYPE_STRING:{
        struct AJString * hold = (struct AJString *)ak->value;
        free(hold->string);
        free(hold);
        break;
      }
      case TYPE_ARRAY:{
        struct AJArray * hold = (struct AJArray *)ak->value;
        DeleteAJArray(hold);
        break;
      }
      case TYPE_OBJECT:{
        struct AJObject * hold = (struct AJObject *)ak->value;
        DeleteAJObject(hold);
        break;
      }

    }

    struct AJKeyValuePair * prev = ak;
    ak = ak->NextAJKVP;
    free(prev);

  }
}

//delete anything
void AJDelete(void * aj, int elementType){
  switch(elementType){
    case TYPE_NUMBER:
    case TYPE_BOOLEAN:
    case TYPE_NULL:{
      free(aj);
      break;
    }
    case TYPE_STRING:{
      free(((struct AJString *)aj)->string);
      free(aj);
      break;
    }

    case TYPE_ARRAY:{
      DeleteAJArray((struct AJArray *)aj);
      break;
    }
    case TYPE_OBJECT:{
      DeleteAJObject((struct AJObject *)aj);
      break;
    }
  }
}

typedef struct AJString AJString;
typedef struct AJNumber AJNumber;
typedef struct AJArray AJArray;
typedef struct AJObject AJObject;
typedef struct AJBoolean AJBoolean;
typedef struct AJNull AJNull;
typedef struct AJArrayElement AJArrayElement;
typedef struct AJKeyValuePair AJKeyValuePair;

// #define TESTING_AROLAN_JSON
#ifdef TESTING_AROLAN_JSON
//small test suite
int main(){
  char * JSONFile = LoadJSONFromFile("test.json");
  AJNumber * nu = CreateAJNumber(11.17f);
  AJString * st = CreateAJString("Eni To Ba Da Omi Siwaju ");
  AJBoolean * bo = CreateAJBoolean("TRUE");
  AJNull * nul = CreateAJNull();

  int hold = 0;
  AJArray * aja = ParseNewAJArray(0, "[{'a':1},{'b':2},{'c':3},{'d':4},{'e':5}]", &hold);

  hold = 0;
  AJObject * ajo = ParseNewAJObject(0, "{ 'key1' :  1, 'key2' : null, 'arraykey' : [{'false':true}], 'objectkey': {'subkey1' : false, 'subkey2' : 20} }", &hold);
  // AJObject * ajo = ParseNewAJObject(0, "{'a':'b'}", &hold);
  // AJObject * ajo = ParseNewAJObject(0, JSONFile, &hold);

  // PrettyPrintAJObject(ajo, 0);

  int len = 1;
  char * buf = (char*)malloc(sizeof(char) * len);
  // memset(buf, '\0', len);

  int bytes_written = 0;
  int pos = 0;
  pos += WriteAJObjectAsStringToBuffer(ajo, &buf, &len, pos) - 1;
  // pos += WriteAJArrayAsStringToBuffer(aja, buf, &len, pos) - 1;

  int len2 = 100;
  char * buf2 = (char*)malloc(sizeof(char) * len);
  int f = 16843009;
  AppendToBuffer_WithKnownLengthOfInput((char*)&f, 4, &buf2, &len2, 0);


  printf("%s\n", buf2);

}
#endif
