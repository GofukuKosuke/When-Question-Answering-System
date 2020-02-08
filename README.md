When-Question-Answering-System
==============================
簡易的なWHEN型質問応答システムである。
複数の入力文書に対して、クエリに関係が深い文書とそこから抽出された日時情報を出力する。
関連性の評価にはTF-IDFを活用している。

## Dependency
- C言語およびコンパイル環境

## Usage
1. 複数の文書ファイルを用意する。
2. when_tfidf.c をコンパイルして、実行ファイル a.out を作成する。
3. ./a.out \[クエリ\] \[コーパスファイル\]の形式で指定する。
  - クエリは1つ以上指定する。複数個可。
  - 文書ファイルも1つ以上指定する。複数個可。ワイルドカードで指定することを推奨。
  - クエリであるか文書ファイルであるかは自動で判断するため、指定する順番は自由である。
    - 具体的には、パスとして存在した場合は文書ファイルとして、存在しなかった場合はクエリとして扱う。
4. 関係が深い文章の情報が3位まで出力される。
  ```
  出力例:
  ./doc100.txt: 1998,1999,
  ./doc085.txt: MAY,
  ./doc222.txt: JULY,1900,
  ```

## Author
御福宏佑(Gofuku Kosuke): [GitHub](https://github.com/GofukuKosuke)
