slangc test.slang -profile vs_6_0 -target spirv -o test-vert.spv -entry vertex_entry
slangc test.slang -profile ps_6_0 -target spirv -o test-frag.spv -entry fragment_entry
pause
