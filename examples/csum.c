int c_sum (int start, int end){
  int accum = 0;
  for(int i=start; i<end; i++)
    accum += i;
  return accum;
}
