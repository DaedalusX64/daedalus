
add_library(HLECombiner_CTR OBJECT 
                    BlendConstant.cpp
                    CombinerExpression.cpp
                    CombinerTree.cpp
                    RenderSettings.cpp
                    )

            
list(APPEND daed_libs "HLECombiner_CTR")
set(daed_libs ${daed_libs} PARENT_SCOPE)
