class HashTest < PicoRubyTest

  desc "Generating a hash"
  assert_equal(<<~RUBY, '{:a=>1, "b"=>"2", :c=>true}')
    hash = {a: 1, "b" => "2", c: true}
    p hash
  RUBY

  desc "empty hash"
  assert_equal(<<~RUBY, "{}")
    p({})
  RUBY

  desc "PICORUBY_HASH_SPLIT_COUNT"
  assert_equal(<<~RUBY, "0\n10\n25\n39\n79")
    hash = {
      k00:  0, k01:  1, k02:  2, k03:  3, k04:  4, k05:  5, k06:  6, k07:  7, k08:  8, k09:  9,
      k10: 10, k11: 11, k12: 12, k13: 13, k14: 14, k15: 15, k16: 16, k17: 17, k18: 18, k19: 19,
      k20: 20, k21: 21, k22: 22, k23: 23, k24: 24, k25: 25, k26: 26, k27: 27, k28: 28, k29: 29,
      k30: 30, k31: 31, k32: 32, k33: 33, k34: 34, k35: 35, k36: 36, k37: 37, k38: 38, k39: 39,
      k40: 40, k41: 41, k42: 42, k43: 43, k44: 44, k45: 45, k46: 46, k47: 47, k48: 48, k49: 49,
      k50: 50, k51: 51, k52: 52, k53: 53, k54: 54, k55: 55, k56: 56, k57: 57, k58: 58, k59: 59,
      k60: 60, k61: 61, k62: 62, k63: 63, k64: 64, k65: 65, k66: 66, k67: 67, k68: 68, k69: 69,
      k70: 70, k71: 71, k72: 72, k73: 73, k74: 74, k75: 75, k76: 76, k77: 77, k78: 78, k79: 79,
    }
    p hash[:k00]
    p hash[:k10]
    p hash[:k25]
    p hash[:k39]
    p hash[:k79]
  RUBY

end
