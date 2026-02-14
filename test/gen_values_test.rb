class GenValuesTest < Picotest::Test

  def test_gen_values_in_generator_c
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      a = ->(m,*rest,m2,**opts,&block) do
        p m,rest,m2,opts;
        block.call
      end
      a.call(1,2,3,4,{a: 0}) {p :hello}
    RUBY
    actual = run_script(script)
    assert_equal("1\n[2, 3, 4]\n{a: 0}\n{}\n:hello", actual)
  end

end
