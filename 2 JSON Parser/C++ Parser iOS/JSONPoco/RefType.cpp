//RefType.cpp
#include "RefType.h"

RefType::RefType(){}

RefType::RefType(Object::Ptr object)
{
    deserialize(object);
}

RefType::~RefType(){
}


string RefType::getNo(){
    return this->mNo;
}

void RefType::setNo(string no){
    this->mNo = no;
}

string RefType::getName(){
    return this->mName;
}

void RefType::setName(string name){
    this->mName = name;
}

std::string RefType::getUrl(){
    return this->mUrl;
}

void RefType::setUrl(string url){
    this->mUrl = url;
}

LocationType* RefType::getLocation(){
    return this->mLocation;
}

void RefType::setLocation(LocationType *location){
    this->mLocation = location;
}

void RefType::deserialize(Object::Ptr object){
    if(object->has(NUMBER) && !object->isNull(NUMBER)){
        std::string str;
        Var no = object->get(NUMBER);
        Object::Ptr subObject = no.extract<Object::Ptr>();
        if (subObject->has("$")&& !subObject->isNull("$")) {
            str = subObject->get("$").convert<std::string>();
        }else{
            str = no.convert<std::string>();
        }
        this->mNo = str;
    }
    if(object->has(NAME) && !object->isNull(NAME)){
        std::string str;
        Var name = object->get(NAME);
        Object::Ptr subObject = name.extract<Object::Ptr>();
        if (subObject->has("$")&& !subObject->isNull("$")) {
            str = subObject->get("$").convert<std::string>();
        }else{
            str = name.convert<std::string>();
        }
        
        this->mName = str;
    }
    if(object->has(URL) && !object->isNull(URL)){
        std::string str;
        Var url = object->get(URL);
        Object::Ptr subObject = url.extract<Object::Ptr>();
        if (subObject->has("$")&& !object->isNull("$")) {
            str = subObject->get("$").convert<std::string>();
        }else{
            str = url.convert<std::string>();
        }
        this->mUrl = str;
    }
    if(object->has(LOCATION) && !object->isNull(LOCATION)){
        Var locationRaw = object->get(LOCATION);
        Object::Ptr locationObject = locationRaw.extract<Object::Ptr>();
        this->mLocation = new LocationType(locationObject);
    }
    else {
        printf("NO! Location");

    }
    
}

