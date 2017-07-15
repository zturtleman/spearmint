uniform sampler2D u_DiffuseMap;

uniform int       u_AlphaTest;
uniform float     u_AlphaTestRef;

varying vec2      var_Tex1;
varying vec4      var_Color;


void main()
{
	vec4 color = texture2D(u_DiffuseMap, var_Tex1);

	float alpha = color.a * var_Color.a;
	if (u_AlphaTest == U_ATEST_EQUAL)
	{
		if (alpha != u_AlphaTestRef)
			discard;
	}
	else if (u_AlphaTest == U_ATEST_GREATEREQUAL)
	{
		if (alpha < u_AlphaTestRef)
			discard;
	}
	else if (u_AlphaTest == U_ATEST_LESS)
	{
		if (alpha >= u_AlphaTestRef)
			discard;
	}
	else if (u_AlphaTest == U_ATEST_LESSEQUAL)
	{
		if (alpha > u_AlphaTestRef)
			discard;
	}
	else if (u_AlphaTest == U_ATEST_NOTEQUAL)
	{
		if (alpha == u_AlphaTestRef)
			discard;
	}
	else if (u_AlphaTest == U_ATEST_GREATER)
	{
		if (alpha <= u_AlphaTestRef)
			discard;
	}
	
	gl_FragColor.rgb = color.rgb * var_Color.rgb;
	gl_FragColor.a = alpha;
}
