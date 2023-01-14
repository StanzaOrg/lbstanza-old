;============================================================
;===================== Primitives ===========================
;============================================================

defatom char (x:Char) :
  writer : (write-byte(#buffer, to-byte(x)))
  reader : (to-char(read-byte(#buffer)))
  size : (1)
    
defatom byte (x:Byte) :
  writer : (write-byte(#buffer, x))
  reader : (read-byte(#buffer))
  size : (1)

defatom int (x:Int) :
  writer :
    stz/var-length-ints/to-var-int(
      x,
      fn (x:Byte) : #write[byte](x))
  reader :
    stz/var-length-ints/from-var-int(
      fn () : #read[byte])
  skip :
    stz/var-length-ints/skip-var-int(
      fn () : #read[byte],
      fn (n:Int) : skip(#buffer, n))

defatom long (x:Long) :
  writer : (write-long(#buffer,x))
  reader : (read-long(#buffer))
  size : (8)

defatom float (x:Float) :
  writer : (write-float(#buffer,x))
  reader : (read-float(#buffer))
  size : (4)

defatom double (x:Double) :
  writer : (write-double(#buffer,x))
  reader : (read-double(#buffer))
  size : (8)

defatom string (x:String) :
  writer :
    #write[length](length(x))
    for c in x do : #write[char](c)
  reader :
    val len = #read[length]
    val str = stz/fastio-runtime/uninitialized-string(len)
    read-chars(str, #buffer)
    str
  skip :
    val len = #read[length]
    skip(#buffer, len)

defatom symbol (x:Symbol) :
  writer : (#write[string](to-string(x)))
  reader : (to-symbol(#read[string]))
  skip : (#skip[string])

defatom bool (x:True|False) :
  writer :
    #write[byte](1Y when x else 0Y)
  reader :
    switch(#read[byte]) :
      0Y : false
      1Y : true
      else : #error
  size :
    1

;============================================================
;======================= Lists ==============================
;============================================================

defcombinator list (item:X) (xs:List<X>) :
  writer :
    #write[length](length(xs))
    for x in xs do :
      #write[item](x)
  reader :
    val accum = Array<X>(#read[length])
    for i in 0 to length(accum) do :
      accum[i] = #read[item]
    to-list(accum)
  skip :
    val len = #read[length]
    for i in 0 to len do : #skip[item]

;============================================================
;===================== KeyValues ============================
;============================================================

defcombinator keyvalue (key:K, value:V) (x:KeyValue<K,V>) :
  writer :
    #write[key](key(x))
    #write[value](value(x))
  reader :
    val key = #read[key]
    val value = #read[value]
    key => value
  skip :
    #skip[key]
    #skip[value]

;============================================================
;====================== Vectors =============================
;============================================================

defcombinator vector (item:T) (xs:Vector<T>) :
  writer :
    #write[length](length(xs))
    for x in xs do : #write[item](x)
  reader :
    val v = Vector<T>()
    for i in 0 to #read[length] do :
      add(v, #read[item])
    v
  skip :
    val len = #read[length]
    for i in 0 to len do : #skip[item]

;============================================================
;====================== Tuples ==============================
;============================================================

defcombinator tuple (item:T) (xs:Tuple<T>) :
  writer :
    #write[length](length(xs))
    for x in xs do : #write[item](x)
  reader :
    val n = #read[length]
    to-tuple(repeatedly({#read[item]}, n))
  skip :
    val len = #read[length]
    for i in 0 to len do :
      #skip[item]

;============================================================
;====================== Optional ============================
;============================================================
defcombinator opt (item:T) (x:T|False) :
  writer :
    match(x) :
      (x:False) :
        #write[byte](0Y)
      (x:T) :
        #write[byte](1Y)
        #write[item](x)
  reader :
    switch(#read[byte]) :
      0Y : false
      1Y : #read[item]
      else : #error
  skip :
    switch(#read[byte]) :
      0Y : false
      1Y : #skip[item]
      else : #error


;============================================================
;====================== Maybe ===============================
;============================================================
defcombinator maybe (item:T) (x:Maybe<T>) :
  writer :
    if empty?(x) :
      #write[byte](0Y)
    else :
      #write[byte](1Y)
      #write[item](value!(x))
  reader :
    switch(#read[byte]) :
      0Y : None()
      1Y : One(#read[item])
      else : #error
  skip :
    switch(#read[byte]) :
      0Y : false
      1Y : #skip[item]
      else : #error

;============================================================
;====================== FileInfo ============================
;============================================================
deftype info (FileInfo) :
  filename:string
  line:int
  column:int

;============================================================
;==================== Byte Arrays ===========================
;============================================================

defatom bytearray (x:ByteArray) :
  writer :
    #write[int](length(x))
    for xi in x do : #write[byte](xi)
  reader :
    val n = #read[length]
    val array = stz/fastio-runtime/uninitialized-byte-array(n)
    read-bytes(array, #buffer)
    array
  skip :
    val n = #read[length]
    skip(#buffer, n)

;============================================================
;================= Checked Quantities =======================
;============================================================

;Eagerly check whether an integer, when read, is of
;reasonable size. We use these lengths to allocate arrays
;for storing the following data, so we don't want to
;cause a segfault due to reading a corrupted file.
defatom length (x:Int) :
  writer :
    #write[int](x)
  reader :
    val x = #read[int]
    #error when x < 0 or x > 8388608
    x
  skip :
    #skip[int]
