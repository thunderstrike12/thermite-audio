slangc test.slang -profile vs_6_0 -target spirv -o test-vert.spv -entry vertex_entry
slangc test.slang -profile ps_6_0 -target spirv -o test-frag.spv -entry fragment_entry

slangc debug-line.slang -profile vs_6_0 -target spirv -o debug-line-vert.spv -entry vertex_entry
slangc debug-line.slang -profile ps_6_0 -target spirv -o debug-line-frag.spv -entry fragment_entry

pause
