Please refer to swiftray/README.md
<!--
# Understanding SVG++ library

這個 Library 功能完整，但由於使用了大量 Template 跟 Metaprogramming，一開始不熟悉 C++ 會非常難 Debug。

要修改這個地方，你會需要知道的概念有：

### 1. C++ Template 基礎
https://www.rocksaying.tw/archives/3641717.html

### 2. #include 與特殊巨集 SVGPP_ON
你會在 SVG++ 裡面看到 \*.inc 檔案，並在其他地方看到 #include "***.inc"，這邊編譯器只會視為一般 ".h" 標頭檔讀入，不是特殊格式。在 #include *.inc 的前後我們會通常會看到 #define SVGPP_ON 或 SVGPP_ON_NS，這邊編譯器會將定義好的巨集代入 *.inc 的內容。

### 3. context_factories 與 ChildContextFactories
這個是 SVG++ 特製的寫法，ChildContextFactories::apply 會是一個 template struct，可以透過 typedef **struct ChildContextFactories::apply<特定 Context, 特定 Element Tag>::type** 這個東西的類別，來使 SVG++ 在特定 Context 遇到特定 Tag 時轉向使用其他 Context。

舉例來說，在 svg_impl.cpp 裡面我們會看到
``
struct ChildContextFactories {
  template <class ParentContext, class ElementTag, class Enable = void>
  struct apply {
    // Default definition handles "svg" and "g" elements
    typedef factory::context::on_stack<BaseContext> type;
  };
};
``

下面定義的
``
template <>
struct ChildContextFactories::apply<BaseContext, tag::element::use_, void> {
  typedef factory::context::on_stack<UseContext> type;
};

``
就是指定 SVG++ 在 BaseContext 遇到 <use> 時轉向 UseContext。
如果 <use> 裡面有 <symbol>，那麼就要另外定義 UseContext 下的 tag::element::symbol 要用哪個 Context。
（很重要）

### 4. Context 裡面的 set() 函數
針對任何 svg attribute 的 set 函數可以參考 **_context.h，若是你新增某個 processed_attribute 但是沒有新增對應的 set 函數，你就會得到一大串的 error，跟你講缺乏某個 set 函數。你通常可以在這個 error 中的 traceback 找到 attribute_id_to_tag.hpp 錯誤的行數，找到那邊你就知道是哪個 tag 的哪個 attribute 沒有被處理到。

### 5. BaseContext 沒有某個 set() 函數，但明明已經設定這個 Element 轉到某個 Context
很可能是這個在定義 **ChildContextFactories::apply<BaseContext, 特定 Element Tag>** 時忘記設定其他可能包含這個 tag 的 Context（例如設定 BaseContext, tag::element::text 卻忘記設定 GroupContext, tag::element::text)

### 6. 新增 attribute
請參考 "data_config_name" 有出現的地方，通通有樣學樣即可，要注意的是 enumerate_all_attributes.inc **一定要按照字母順序排列！！**，跟 cariosvg 一樣，不然 debug 會很崩潰。
-->