MRuby::Gem::Specification.new('mruby-compiler2') do |spec|
  spec.license = 'MIT'
  spec.author  = 'HASUMI Hitoshi'
  spec.summary = 'mruby compiler using a universal parser'

  lib_dir = "#{dir}/lib"
  cc.include_paths << "#{dir}/include"

  prism_dir = "#{lib_dir}/prism"
  ruby_dir = "#{lib_dir}/ruby"

  if cc.defines.flatten.include? "MRC_PARSER_PRISM"
    objs.delete_if {|obj| obj =~ /lrama_helper/}
    prism_templates_dir = "#{lib_dir}/prism/templates"
    cc.include_paths << "#{prism_dir}/include"

    task :deep_clean do
      rm_rf prism_dir
    end

    next if Rake.application.top_level_tasks.first == "deep_clean"

    directory prism_dir do
      FileUtils.cd lib_dir do
        sh "git clone https://github.com/ruby/prism.git"
      end
      FileUtils.cd prism_dir do
        sh "git checkout v0.30.0"
      end
    end

    task :prism_templates => prism_dir do
      FileUtils.cd prism_dir do
        sh "templates/template.rb"
      end
    end

    TEMPLATE_GENERATES = %w(node prettyprint serialize token_type)

    TEMPLATE_GENERATES.each do |name|
      dst = "#{prism_dir}/src/#{name}.c"
      # file task does not work when dst does not exist. why?
      Rake::Task[:prism_templates].invoke unless File.exist?(dst)
      file dst => ["#{prism_templates_dir}/src/#{name}.c.erb", "#{prism_templates_dir}/template.rb"] do |t|
        Rake::Task[:prism_templates].invoke
      end
    end

    Dir.glob("#{prism_dir}/src/**/*.c").map do |src|
      obj = objfile(src.pathmap("#{build_dir}/lib/%n"))
      objs << obj
      file obj => [src] do |f|
        cc.run f.name, f.prerequisites.first
      end
    end
  elsif cc.defines.flatten.include? "MRC_PARSER_LRAMA"
    cc.defines << "UNIVERSAL_PARSER"
    cc.include_paths << "#{prism_dir}/include" # for pm_constant_pool
    cc.include_paths << "#{ruby_dir}"
    cc.include_paths << "#{ruby_dir}/include"
    cc.include_paths << "#{ruby_dir}/.ext/include/x86_64-linux"
    [
      "#{prism_dir}/src/util/pm_constant_pool.c",
      "#{ruby_dir}/parse.c",
      "#{ruby_dir}/node.c",
      "#{ruby_dir}/parser_st.c"
    ].each do |src|
      objs << objfile(src.pathmap("#{build_dir}/lib/%n"))
      file objs.last => [src] do |f|
        cc.run f.name, f.prerequisites.first
      end
    end
  else
    raise "You have to specify MRC_PARSER_PRISM or MRC_PARSER_LRAMA"
  end

end

