<?
//  Copyright 2013, Tera Insights. All rights reserved

/* A Json object container so we can produce JSON data out of the system */

function JSON(array $t_args) {
    $debug = get_default($t_args, 'debug', 0);

?>

class JSON {
public:
    typedef Json::Value inner_type;

private:
    // pointer to inner_type object
    inner_type* data;

public:
    // constructor from Json::value; steals the content so call it only when done
    JSON(inner_type& _data);
    JSON & operator =(inner_type& data);

    // default constructor
    JSON(void):data(new inner_type){};

    // methods to access object
    const inner_type& get(void) const{ return *data; }
    // set steals the content
    void set(inner_type& obj);

    void FromString(const char* text);
    int ToString(char* text) const;

    void fromJson( const Json::Value & src );
    void toJson( Json::Value & dest ) const;

    uint64_t GetId(void) const { return reinterpret_cast<uint64_t>(data); }

    void copy(const JSON& other){ (*data) = *(other.data); }

    void Destroy(void);

    ~JSON(void){ }
};

inline
JSON :: JSON(inner_type& _data){
    // create new internal value and steal the content of the argument
    data = new inner_type;
    data->swap(_data);
}

inline
JSON & JSON :: operator = (inner_type & data) {
    set(data);

    return *this;
}

inline
void JSON :: set(inner_type& obj) {
    data = new inner_type;
    data->swap(obj);
}

inline
void JSON :: FromString(const char* text){
    std::stringstream st(text);
    Json::Reader rd;
    rd.parse(st, *data, false);
}

inline
int JSON :: ToString(char* text) const {
    Json::FastWriter fw;
    std::string str = fw.write(*data);
    strcpy(text, str.c_str());
    size_t size = str.size();
    // FastWriter adds a newline to the end of the text. Remove it.
    text[size] = '\0';
    return size;
}

inline
void JSON :: fromJson( const Json::Value & src ) {
    if( data == nullptr ) {
        data = new inner_type;
    }

    *data = src;
}

inline
void JSON :: toJson( Json::Value & dest ) const {
    if( data != nullptr )
        dest = *data;
}

inline
void JSON :: Destroy(void) {
    if (data) delete data;
    data = nullptr;
}

<? ob_start(); ?>


inline void FromString(@type & x, const char* text){
    x.FromString(text);
}

inline int ToString(const @type & x, char* text){
    return x.ToString(text);
}

inline
void ToJson( const @type & src, Json::Value & dest ) {
    src.toJson(dest);
}

inline
void FromJson( const Json::Value & src, @type & dest ) {
    dest.fromJson(src);
}

// The hash function
// we just use conversion to unsigned int
inline uint64_t Hash(const @type x){ return x.GetId();}


// Deep copy
inline
void Copy( @type & to, const @type & from ) {
    to.copy(from);
}

<? $gContent = ob_get_clean(); ?>

<?
    return array(
        'kind'           => 'TYPE',
        "system_headers" => array ( "string", "sstream", "inttypes.h", 'jsoncpp/json/json.h' ),
        'libraries'      => [ 'jsoncpp' ],
        "complex"        => false,
        "global_content" => $gContent,
        'properties'     => [],
        'destroy'        => true,
        'describe_json'  => DescribeJson('json'),
    );
}

?>
