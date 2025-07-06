class GenValuesTest < PicoRubyTest

  if @@vm_select == :mruby

    desc "gen_values() in generator.c"
    assert_equal(<<~RUBY, (@@vm_select == :mruby ? "1\n[2, 3, 4]\n{a: 0}\n{}\n:hello" : "1\n[2, 3, 4]\n{:a=>0}\n{}\n:hello"))
      a = ->(m,*rest,m2,**opts,&block) do
        p m,rest,m2,opts;
        block.call
      end
      a.call(1,2,3,4,{a: 0}) {p :hello}
    RUBY

  end

end
