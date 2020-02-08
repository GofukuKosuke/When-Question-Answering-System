/*******************************************************************************
WHEN型質問応答システム
*******************************************************************************/

#include <stdio.h> /* 標準入出力 */
#include <stdlib.h> /* 標準ライブラリ */
#include <string.h> /* 文字列 */
#include <ctype.h> /* 文字判定&変換 */
#include <stdbool.h> /* 真偽値(bool) */
#include <math.h> /* 数学 */

/* 定数 */
#define BUFFER_LENGTH 1024 /* 行バッファの長さ */
#define DELIMITER " .,:;?!()" /* 区切り文字一覧 */
#define DISABLE_DF false /* DFを考慮しない(デバッグモード) */
#define MAX_SEARCH_DOCUMENT_COUNT 3 /* 検索をかける最大文書数 */
#define TIMETYPE_YEAR 1
#define TIMETYPE_MONTH 2
#define TIMETYPE_DAY 3

/* グローバル変数 */
int query_count = 0; /* クエリの個数 */
int document_count = 0; /* 文書の個数 */

/* クエリを格納する構造体(単方向線形リスト) */
typedef struct _query {
  char *str; /* String: クエリ文字列 */
  bool flag; /* 文書にそのクエリが含まれているか判定するために用いるフラグ、DFの算出に用いる */
  int df; /* DocumentFrequency */
  struct _query *next;
} Query;

/* 読み込んだ文書を格納する構造体(単方向線形リスト) */
typedef struct _document {
  char *fname; /* FileName: ファイル名、argvを参照するだけ */
  FILE *fp; /* FilePointer: ファイルポインタ */
  int wc; /* WordCount: 文書内の単語の個数 */
  int *tf; /* TermFrequency: クエリに対するTFを正規化せずに保存する、クエリの個数ぶんだけ動的確保する */
  double avg_tfidf; /* Average_TF-IDF: 各々のクエリに対して、TF-IDFを計算したあと平均をとったもの */
  struct _document *next;
} Document;

/* プロトタイプ宣言 */
void str_toupper(char*);

/* Queryリスト & Query配列 */
Query* new_query(char*);
Query** make_query_array(Query*);
void clear_query_flags(Query**);
void free_queries(Query**);

/* Documentリスト & Document配列 */
Document* new_document(char*, FILE*);
Document** make_document_array(Document*);
void malloc_tf(Document**);
void swap_2_document_nodes(Document*, Document*);
void sort_documents(Document**);
void free_documents(Document**);

/* TF-IDF */
double calc_tfidf(int, int, int);
void calc_all_avg_tfidf(Document**, Query**);

/* ログ出力 */
void output_wc(Document**);
void output_tf(Document**, Query**);
void output_df(Query**);
void output_avg_tfidf(Document**);

/* 時間の判定 */
bool str_is_only_digits(char*);
bool is_year(char*);
bool is_month(char*);
bool is_day(char*);
int time_type(char*);

int main ( int argc, char *argv[] ) {
  /* 一時変数 */
  FILE *fp;
  char buffer[BUFFER_LENGTH];
  char *s; /* 文字列ポインタ(strtokの処理などで用いる) */
  int i, j; /* ループカウンタ */

  /* リスト(ダミーノードも作成) */
  Query *query_top = new_query(""), *current_query;
  Document *document_top = new_document("", NULL), *current_document;

  /* リストの連続領域(ポインタ配列) */
  Query **queries;
  Document **documents;

  /* 引数の個数判定
    実行ファイル名 + クエリ1つ以上 + 参照ファイル1つ以上 = 計3つ以上のargcが必要 */
  if ( argc < 3 ) {
    printf("引数の数が違います。\n");
    return 1;
  }

  /* リストのポインタをセット */
  current_query = query_top;
  current_document = document_top;

  /* 引数を全て取得 */
  for ( i = 1; i < argc; i++ ) {
    fp = fopen(argv[i], "r");

    /* ファイルでなければ、クエリとして扱う */
    if ( fp == NULL ) {
      current_query->next = new_query(argv[i]);
      current_query = current_query->next;
      query_count++;
    }

    /* ファイルを読み込む */
    else {
      current_document->next = new_document(argv[i], fp);
      current_document = current_document->next;
      document_count++;
    }
  }

  /* クエリが1つ以上あるか確認 */
  if ( query_count < 1 ) {
    printf("クエリがありません。\n");
    return 1;
  }

  /* 文書が1つ以上あるか確認 */
  if ( document_count < 1 ) {
    printf("文書がありません。\n");
    return 1;
  }

  /* 連続ポインタ領域の確保 */
  queries = make_query_array(query_top);
  documents = make_document_array(document_top);

  malloc_tf(documents); /* 各々のDocument要素にTFの領域を確保 */

  /* 読み込んだ文書を全て処理 */
  for ( i = 0; i < document_count; i++ ) {
    clear_query_flags(queries); /* クエリのフラグを初期化 */

    /* 一行ずつ読み込む */
    while( fgets(buffer, BUFFER_LENGTH, documents[i]->fp) != NULL ) {
      buffer[ strlen(buffer) - 1 ] = '\0'; /* 改行コードを消す */
      str_toupper(buffer); /* 全て大文字に変換 */

      /* 単語に区切って処理する */
      s = strtok(buffer, DELIMITER);
      while ( s != NULL ) {

        documents[i]->wc++; /* 単語を数えて格納する */

        for ( j = 0; j < query_count; j++ ) {
          /* 各々のクエリと比較しながら、TFを数え、フラグを立てる */
          if ( strcmp(s, queries[j]->str) == 0 ) {
            documents[i]->tf[j]++;
            queries[j]->flag = true;
          }
        }

        s = strtok(NULL, DELIMITER);
      }
    }

    /* クエリのフラグを元に、DFを数える */
    for ( j = 0; j < query_count; j++ ) {
      queries[j]->df += (int)queries[j]->flag; /* フラグがtrueならインクリメントされる */
    }
  }

  calc_all_avg_tfidf(documents, queries); /* 平均TF-IDFの計算 */
  sort_documents(documents); /* 平均TF-IDF降順に文書をソート */

  /* ログファイル書き出し */
  output_wc(documents);
  output_tf(documents, queries);
  output_df(queries);
  output_avg_tfidf(documents);

  /* 単語の検索(MAX_SEARCH_DOCUMENT_COUNTまでで打ち切る) */
  for (i = 0; (i < MAX_SEARCH_DOCUMENT_COUNT && i < document_count); i++ ) {

    /* 平均TF-IDFが0なら、打ち切り */
    if ( documents[i]->avg_tfidf < 0.000001 ) {
      break;
    }

    printf("%s: ", documents[i]->fname);
    fseek(documents[i]->fp, 0L, SEEK_SET); /* ファイルポインタを先頭へ */
    while( fgets(buffer, BUFFER_LENGTH, documents[i]->fp) != NULL ) {
      buffer[ strlen(buffer) - 1 ] = '\0'; /* 改行コードを消す */
      str_toupper(buffer); /* 全て大文字に変換 */

      /* 単語に区切って処理する */
      s = strtok(buffer, DELIMITER);
      while ( s != NULL ) {
        /* 時間単語のみを出力 */
        if (time_type(s) != 0) {
          printf("%s,", s);
        }
        s = strtok(NULL, DELIMITER);
      }
    }
    printf("\n");
  }

  /* 平均TF-IDFによって打ち切られたときの表示処理 */
  if ( i == 0 ) {
    printf("クエリに関連する文書が見つかりませんでした。\n");
  }

  /* 終了処理(資源の解放) */
  free_queries(queries);
  free_documents(documents);
  return 0;

}

/* 文字列を全て大文字にする */
void str_toupper(char *s) {
  while ( *s != '\0' ) {
    *s = toupper(*s);
    s++;
  }
}

/* 新しいQueryノードを作り、返す */
Query* new_query(char *str) {

  /* クエリ文字列(大文字)の領域を確保 */
  char *pt_str = (char *)malloc( sizeof(char) * (strlen(str) + 1) );
  strcpy(pt_str, str);
  str_toupper(pt_str);

  /* 新しい1つのQueryノードを確保 */
  Query *new_node = (Query *)malloc( sizeof(Query) );
  new_node->str = pt_str;
  new_node->df = 0;
  new_node->next = NULL;
  return new_node;

}

/* 新しいQueryの連続ポインタ領域を作る */
Query** make_query_array(Query *q) {
  int i;
  Query** new_array = (Query**)malloc(sizeof(Query**)*query_count);
  q = q->next; //先頭のノードをとばす
  for(i=0;i<query_count;i++){
    new_array[i] = q;  //ポインタ配列からqueryを指す
    q = q->next;  //次のqueryを指す
  }
  return new_array;
}

/* Queryの中にあるフラグを全て0クリアする */
void clear_query_flags(Query **q) {
  int i;
  for (i = 0; i < query_count; ++i) {
    	q[i]->flag=false;  //全てのq[i]->flagを0クリアする
	}
}

/* 全てのQueryノードを解放する */
void free_queries(Query **q) {
  int i;
  for ( i = 0; i < query_count; i++ ) {
    free(q[i]->str);
    free(q[i]);
  }
  free(q);
}

/* 新しいDocumentノードを作り、返す (TFの領域はこの関数では確保しない) */
Document* new_document(char *fname, FILE *fp) {
  Document *new_node = (Document *)malloc( sizeof(Document) * 1 );
  new_node->fname = fname;
  new_node->fp = fp;
  new_node->wc = 0;
  new_node->avg_tfidf = 0.0;
  new_node->next = NULL;
  return new_node;
}

/* 新しいDocumentの連続ポインタ領域を作る */
Document** make_document_array(Document *d) {
  int i;
  Document** new_array = (Document**)malloc(sizeof(Document**)*document_count);
  d = d->next; //先頭のノードをとばす
  for(i=0;i<document_count;i++){
    new_array[i] = d;  //ポインタ配列からdocumentを指す
    d = d->next;  //次のdocumentを指す
  }
  return new_array;
}

/* 全てのDocumentノードに、TFを格納する領域を生成する */
void malloc_tf(Document **d) {
  int i, j;
  for ( i = 0; i < document_count; i++ ) {
    d[i]->tf = (int *)malloc( sizeof(int) * query_count );
    /* 0クリア */
    for ( j = 0; j < query_count; j++ ) {
      d[i]->tf[j] = 0;
    }
  }
}

/* 2つのDocumentノードを入れ替える (注意: ノードの順序関係は崩壊します) */
void swap_2_document_nodes(Document *d1, Document *d2) {
  Document temp = *d1;
  *d1 = *d2;
  *d2 = temp;
}

/* Document配列をavg_tfidf降順で選択ソートする(ノードの順序関係も再構築する) */
void sort_documents(Document **d) {
  int i, j, max;

  /* 選択ソート */
  for ( i = 0; i < document_count; i++ ) {
    max = i;
    for ( j = i+1; j < document_count; j++ ) {
      if ( d[j]->avg_tfidf > d[max]->avg_tfidf ) {
        max = j;
      }
    }
    if ( max != i ) {
      swap_2_document_nodes(d[i], d[max]);
    }
  }

  /* ノードの順序関係の再構築 */
  for ( i = 0; i < document_count-1; i++ ) {
    d[i]->next = d[i+1];
  }
  d[i]->next = NULL;

}

/* 全てのDocumentノードを解放する(ファイルも同時に解放する) */
void free_documents(Document **d) {
  int i;
  for ( i = 0; i < query_count; i++ ) {
    fclose(d[i]->fp);
    free(d[i]->tf);
    free(d[i]);
  }
  free(d);
}

/* TF-IDFを計算する */
double calc_tfidf(int tf, int wc, int df) {
  double ntf = (double)tf / wc; /* NormalizedTF: 正規化TF */
  double idf;
  /* デバッグ時 or DF=0のとき、 IDFの影響を無効化 */
  if ( DISABLE_DF || df == 0 ) {
    idf = 1.0;
  } else {
    idf = log((double)document_count) - log((double)df) + 1.0;
  }
  return ntf * idf;
}

/* 全ての平均TF-IDFを計算して格納する */
void calc_all_avg_tfidf(Document **d, Query **q) {
  int i, j;
  for ( i = 0; i < document_count; i++ ) {
    for ( j = 0; j < query_count; j++ ) {
      d[i]->avg_tfidf += calc_tfidf(d[i]->tf[j], d[i]->wc, q[j]->df);
    }
    d[i]->avg_tfidf /= query_count;
  }
}

/* 文書ごとの単語数をログファイルに出力 */
void output_wc(Document **d) {
  int i;
  FILE *fp = fopen("wc.log", "w");
  for ( i = 0; i < document_count; i++ ) {
    fprintf(fp, "%s: %4d\n", d[i]->fname, d[i]->wc);
  }
  fclose(fp);
}

/* 文書ごとのTF(正規化無し)をログファイルに出力 */
void output_tf(Document **d, Query **q) {
  int i, j;
  FILE *fp = fopen("tf.log", "w");
  for ( i = 0; i < document_count; i++ ) {
    fprintf(fp, " %s: ", d[i]->fname);
    for ( j = 0; j < query_count; j++ ) {
      fprintf(fp, "(%s:%4d), ", q[j]->str, d[i]->tf[j]);
    }
    fprintf(fp, "\n");
  }
  fclose(fp);
}

/* クエリごとのDFをログファイルに出力 */
void output_df(Query **q) {
  int i;
  FILE *fp = fopen("df.log", "w");
  for ( i = 0; i < query_count; i++ ) {
    fprintf(fp, "%s: %3d\n", q[i]->str, q[i]->df);
  }
  fclose(fp);
}

/* 文書ごとの平均TF-IDFをログファイルに出力 */
void output_avg_tfidf(Document **d) {
  int i;
  FILE *fp = fopen("avg_tfidf.log", "w");
  for ( i = 0; i < document_count; i++ ) {
    fprintf(fp, "%s: %7.4f\n", d[i]->fname, d[i]->avg_tfidf);
  }
  fclose(fp);
}

/* 文字列が数字のみかどうか判定する */
bool str_is_only_digits(char *s) {
  while ( *s != '\0' ) {
    if ( !isdigit(*s) ) {
      return false;
    }
    s++;
  }
  return true;
}

/* 文字列が年(4桁の数字)かどうか判定する */
bool is_year(char *s){
  int digit;
  if (!str_is_only_digits(s)) {
    return false;
  }
  digit = atoi(s);
  if ( 1000 <= digit && digit <= 9999 ) {
    return true;
  }
  return false;
}

/* 文字列が月かどうか判定する */
bool is_month(char *s){
	char *month[12]=
    {"JANUARY","FEBRUARY","MARCH","APRIL","MAY",
    "JUNE","JULY","AUGUST","SEPTEMBER","OCTOBER",
    "NOVEMBER","DECEMBER"};
	int i=0;
	char *m;
	while (i<12){
		m=month[i];
		if(strcmp(s,m)==0){
            return true;
         }
		i++;
	}
	return false;
}

/* 文字列が曜日かどうか判定する */
bool is_day(char *s){
	char *day[7]=
    {"SUNDAY","MONDAY","TUESDAY","WEDNESDAY",
    "THURSDAY","FRIDAY","SATURDAY"};
	int i=0;
	char *d;
	while (i<7){
		d=day[i];
		 if(strcmp(s,d)==0){
            return true;
         }
		i++;
	}
	return false;
}

/* 文字列の時間形式を判定する
  時間でなかったら0を返す */
int time_type(char *s){
	if((is_year(s))){
		return TIMETYPE_YEAR;
	}
	if(is_month(s)){
		return TIMETYPE_MONTH;
	}
	if(is_day(s)){
		return TIMETYPE_DAY;
	}
	return 0;
}
