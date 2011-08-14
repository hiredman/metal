#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** TYPES **/
#define NIL_TYPE 0
#define SYMBOL_TYPE 1
#define CONS_TYPE 2
#define EOF_TYPE 3
#define INT_TYPE 4
#define STRINGBUFFER_TYPE 5
#define NOT_FOUND_TYPE 6

typedef union {
  void* ref;
  int num;
} Form_u;

typedef struct {
  int type;
  Form_u ref;
} Form_t;

typedef struct {
  char* (*to_string)(Form_t);
  int (*eq)(Form_t, Form_t);
} Object_t;

typedef struct {
  char* (*to_string)(Form_t);
  int (*eq)(Form_t, Form_t);
  char* name;
} Symbol_t;

typedef struct {
  char* (*to_string)(Form_t);
  int (*eq)(Form_t, Form_t);
  Form_t car;
  Form_t cdr;
} Cons_t;

typedef struct {
  char* (*to_string)(Form_t);
  int (*eq)(Form_t, Form_t);
  char* buf;
  int buf_location;
  int buf_size;
} StringBuffer_t;

Form_t NIL;
/** end TYPES**/

Form_t read(FILE *fp, Form_t eof);
Form_t car(Form_t form);
Form_t cdr(Form_t form);
Form_t print(Form_t form);
char* to_string(Form_t form);

char* symbol_to_string(Form_t f) {
  Symbol_t *s=f.ref.ref;
  return s->name;
}

char* cons_to_string(Form_t f){
  char* car_str = to_string(car(f));
  char* cdr_str = to_string(cdr(f));
  char* buf = malloc(strlen(car_str)+strlen(cdr_str)+6);
  sprintf(buf,"(%s . %s)", car_str, cdr_str);
  buf[strlen(car_str)+strlen(cdr_str)+65]='\0';
  return buf;
}

char* string_buffer_to_string(Form_t form){
  StringBuffer_t* sb;
  char* buf;
  sb=form.ref.ref;
  buf=malloc(sb->buf_location+(1 * sizeof(char)));
  strncpy(buf,sb->buf,sb->buf_location);
  return buf;
}

Form_t string_buffer(){
  Form_t f;
  StringBuffer_t* sb = malloc(sizeof(StringBuffer_t));
  f.type=STRINGBUFFER_TYPE;
  f.ref.ref=sb;
  sb->buf=malloc(sizeof(char)*512);
  sb->buf_location=0;
  sb->buf_size=512;
  sb->to_string=&string_buffer_to_string;
  return f;
}

void append_str(Form_t form, char* chars){
  StringBuffer_t* sb;
  char* new_buf;
  int new_size;
  sb=form.ref.ref;
  if((sb->buf_size - sb->buf_location) < strlen(chars)) {
    new_size=sb->buf_size*2;
    new_buf = realloc(sb->buf, new_size);
    if(new_buf != sb->buf)
      free(sb->buf);
    sb->buf=new_buf;
    sb->buf_size=new_size;
  }
  strncpy(sb->buf+sb->buf_location, chars, strlen(chars));
  sb->buf_location=sb->buf_location+strlen(chars);
}

char* list_to_string(Form_t f){
  Cons_t* cell;
  cell=f.ref.ref;
  char* buf;
  char* buf1;
  Form_t sb;
  Form_t ff;
  int buf_size=0;
  if(cell->cdr.type != CONS_TYPE  && cell->cdr.type != NIL_TYPE){
    return cons_to_string(f);
  } else {
    sb=string_buffer();
    for(ff=f; ff.type!=NIL_TYPE; ff=cdr(ff)) {
      append_str(sb,strdup(" "));
      append_str(sb,to_string(car(ff)));
    }
    buf = to_string(sb);
    buf1 = malloc(strlen(buf)+sizeof(char));
    sprintf(buf1,"(%s)", buf+1);
    return buf1;
  }
}

Form_t integer(int i) {
  Form_t f;
  f.type=INT_TYPE;
  f.ref.num=i;
  return f;
}

int symbol_equal(Form_t a, Form_t b){
  Symbol_t *s1;
  Symbol_t *s2;
  s1=a.ref.ref;
  s2=b.ref.ref;
  if(a.type != b.type)
    return 0;
  if(strcmp(s1->name,s2->name) == 0)
    return 1;
  else
    return 0;
}

Form_t symbol(char* name){
  Form_t f;
  Symbol_t *s;
  f.type=SYMBOL_TYPE;
  s=malloc(sizeof(Symbol_t));
  s->name=strdup(name);
  s->to_string=&symbol_to_string;
  f.ref.ref=s;
  return f;
}

Form_t cons(Form_t car, Form_t cdr){
  Form_t form;
  Cons_t *cell;
  cell = malloc(sizeof(Cons_t));
  cell->car=car;
  cell->cdr=cdr;
  cell->to_string=&list_to_string;
  form.ref.ref=cell;
  form.type=CONS_TYPE;
  return form;
}

Form_t car(Form_t form){
  Cons_t *cell;
  if(form.type == NIL_TYPE) {
    return form;
  } else {
    cell=form.ref.ref;
    return cell->car;
  }
}

Form_t cdr(Form_t form){
  Cons_t *cell;
  if(form.type == NIL_TYPE) {
    return form;
  } else {
    cell=form.ref.ref;
    return cell->cdr;
  }
}

Form_t reverse(Form_t lst){
  Form_t form;
  for(form=NIL; lst.type!=NIL.type; lst=cdr(lst))
    form=cons(car(lst), form);
  return form;
}

/* #define NUM_THREADS     5 */

/* void *PrintHello(void *threadid) */
/* { */
/*   long tid; */
/*   tid = (long)threadid; */
/*   printf("Hello World! It's me, thread #%ld!\n", tid); */
/*   pthread_exit(NULL); */
/* } */

Form_t read_symbol(int c, FILE *fp) {
  char buf[1024];
  int pos = 0;
  buf[pos]=c;
  pos++;
  while((c = fgetc(fp)) != EOF && (c > 96 && c < 172) || (c > 64 && c < 133)
        || c == 33)
  {
    buf[pos]=c;
    pos++;
  }
  buf[pos] = 0;
  ungetc(c, fp);
  return symbol(buf);
}

Form_t read_number(int c, FILE *fp) {
  char buf[1024];
  int pos = 0;
  int i=0;
  char* s;
  Form_t f;
  buf[pos]=c;
  pos++;
  while((c = fgetc(fp)) != EOF && isdigit(c)) {
    buf[pos]=c;
    pos++;
  }
  buf[pos] = 0;
  ungetc(c, fp);
  for(s=buf;(*s != '\0') && isdigit(*s);s++){
    i = i * 10 + *s - '0';
  }
  f = integer(i);
  return f;
}

Form_t read_list(FILE *fp) {
  Form_t lst=NIL;
  Form_t eof;
  int c;
  c=fgetc(fp);
  eof.type=EOF_TYPE;
  while((char)c != ')') {
    while(c > -1 && c < 33 && c != EOF) {
      c=fgetc(fp);
    }
    ungetc(c,fp);
    lst=cons(read(fp,eof), lst);
    c = fgetc(fp);
  }
  return reverse(lst);
}

char* to_string(Form_t f) {
  Object_t *o;
  char* buf;
  int i;
  switch(f.type){
  case INT_TYPE:
    buf=malloc(sizeof(char) * 1024);
    i=sprintf(buf,"%d",f.ref.num);
    buf[i] = '\0';
    return buf;
    break;
  case NIL_TYPE:
    return strdup("nil");
    break;
  default:
    o=f.ref.ref;
    return o->to_string(f);
    break;
  }
}

Form_t print(Form_t f) {
  printf("%s", to_string(f));
  return NIL;
}

Form_t read(FILE *fp, Form_t eof) {
  int c;
  Form_t f;
  c=fgetc(fp);
  while(c > -1 && c < 33 && c != EOF) {
    c=fgetc(fp);
  }
  if (c == EOF) {
    return eof;
  } else if((c > 96 && c < 172) || (c > 64 && c < 133)) {
    return read_symbol(c, fp);
  } else if(isdigit(c)) {
   f = read_number(c, fp);
   return f;
  } else if ((char)c == '(') {
   f=read_list(fp);
   return f;
  } else {
    printf("unknown character %c (%d)\n", c, c);
  }
}

int nilp(Form_t f) {
  if (f.type == NIL_TYPE)
    return 1;
  else
    return 0;
}

Form_t sum_ints(Form_t stack){
  Form_t f, arg1, arg2;
  arg2 = car(stack);
  stack=cdr(stack);
  arg1=car(stack);
  stack=cdr(stack);
  f=integer(arg1.ref.num+arg2.ref.num);
  stack=cons(f,stack);
  return stack;
}

Form_t extend(Form_t env, Form_t name, Form_t value) {
  return cons(cons(name,value), env);
}

Form_t lookup(Form_t env, Form_t name, Form_t notfound){
  Form_t ff,current;
  Form_t f = notfound;
  for(ff=env;ff.type!=NIL_TYPE;ff=cdr(ff)){
    current=car(ff);
    if(symbol_equal(car(current), name) == 1){
      f=cdr(current);
      break;
    }
  }
  return f;
}

Form_t secd(Form_t stack,
            Form_t env,
            Form_t ctl,
            Form_t dump){
  int cont=1;
  Form_t f, arg1, arg2, notfound, r1, r2, r3;
  notfound.type=NOT_FOUND_TYPE;
  env=extend(env, symbol("x"), integer(1));
 start:
    
    if(nilp(ctl) == 1 && nilp(dump) == 1)
      return car(stack);
    
    if (nilp(dump) != 1 && nilp(ctl) == 1) {
      arg1=car(dump);
      r1=car(arg1); //stack
      r2=car(cdr(arg1)); //env
      r3=car(cdr(cdr(arg1))); //ctl
      dump=cdr(dump);
      stack=cons(car(stack), r1);
      env=r2;
      ctl=r3;
      goto start;
    }

    f=car(ctl);
    ctl=cdr(ctl);
    switch(f.type) {
    case INT_TYPE:
      stack=cons(f,stack);
      goto start;
      break;

    case SYMBOL_TYPE:
      arg1=lookup(env, f, notfound);
      if(arg1.type != notfound.type) {
        stack=cons(arg1, stack);
      } else if(symbol_equal(f,symbol("plus"))) {
        stack=sum_ints(stack);
        ctl=cdr(ctl);
      } else if(symbol_equal(f,symbol("apply"))) {
        arg1=car(stack);
        stack=cdr(stack);
        r1=car(arg1);
        arg1=cdr(arg1);
        r2=reverse(car(arg1));
        arg1=cdr(arg1);
        for(f=r2;f.type!=NIL_TYPE;f=cdr(f)){
          r1=extend(r1,car(f),car(stack));
          stack=cdr(stack);
        }
        dump=cons(cons(stack, cons(env, cons(ctl, NIL))), dump);
        env=r1;
        ctl=arg1;          
        stack=NIL;
      }
      goto start;
      break;

    case CONS_TYPE:
      arg1=car(f);
      if(arg1.type == SYMBOL_TYPE && symbol_equal(arg1,symbol("set!")) == 1){
        arg1=car(cdr(f));
        arg2=car(cdr(cdr(f)));
        env=extend(env,arg1,arg2);
      } else if(arg1.type == SYMBOL_TYPE && symbol_equal(arg1,symbol("lambda")) == 1){
        arg1 = cons(env, cons(car(cdr(f)), cdr(cdr(f))));
        stack=cons(arg1, stack);
      } else {
        arg2=reverse(cdr(f));
        ctl = cons(arg1,cons(symbol("apply"), ctl));
        for(f=arg2;f.type != NIL_TYPE;f=cdr(f))
          ctl = cons(car(f),ctl);
      }
      goto start;
      break;

    default:
      stack=cons(f,stack);
      goto start;
      break;
    }
}

int main (int argc, char *argv[])
{
  NIL.type=NIL_TYPE;
  /* pthread_t threads[NUM_THREADS]; */
  /* int rc; */
  /* long t; */
  /* for(t=0; t<NUM_THREADS; t++){ */
  /*   printf("In main: creating thread %ld\n", t); */
  /*   rc = pthread_create(&threads[t], NULL, PrintHello, (void *)t); */
  /*   if (rc){ */
  /*     printf("ERROR; return code from pthread_create() is %d\n", rc); */
  /*     exit(-1); */
  /*   } */
  /* } */
  /* pthread_exit(NULL); */

  FILE *fp;
  Form_t eof;
  Form_t f;
  Form_t code = NIL;
  eof.type=EOF_TYPE;
  f.type=NIL_TYPE;
  fp = fopen(argv[1], "r");
  while(f.type != eof.type) {
    f = read(fp, eof);
    if(f.type != eof.type) {
      printf("return from read\n");
      print(f);
      printf("\n");
      code=cons(f, code);
    }
  }
  fclose(fp);
  f=secd(NIL, NIL,reverse(code), NIL);
  printf("result: %s\n", to_string(f));
  return 0;
}
