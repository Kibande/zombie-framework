; make sure you update aemstro_as for this (15/11/14)
 
; setup constants
	.const 20, 1.0, 0.0, 0.00392, 1.0
 
; setup outmap
	.out o0, result.position
	.out o1, result.color
	.out o2, result.texcoord0
	.out o3, result.texcoord1
	.out o4, result.texcoord2

; setup uniform map (required to use SHDR_GetUniformRegister)
	.uniform 0, 3, projection      ; c0-c3 = projection matrix
	.uniform 4, 7, modelview       ; c4-c7 = modelview matrix
	.uniform 8, 8, sunDirection    ; c8    = sun direction vector
	.uniform 9, 9, sunAmbient      ; c9    = sun ambient color
	.uniform 10, 10, sunDiffuse    ; c10   = sun diffuse color
 
;code
	main:
		mov r1,  v0       (0x6)
		mov r1, c20       (0x3)
		; tempreg = mdlvMtx * in.pos
		dp4 r0,  c4,  r1  (0x0)
		dp4 r0,  c5,  r1  (0x1)
		dp4 r0,  c6,  r1  (0x2)
		mov r0, c20       (0x3)
		; result.pos = projMtx * tempreg
		dp4 o0,  c0,  r0  (0x0)
		dp4 o0,  c1,  r0  (0x1)
		dp4 o0,  c2,  r0  (0x2)
		dp4 o0,  c3,  r0  (0x3)
		; result.texcoord = in.texcoord
		mov o2,  v1       (0x5)
		mov o3, c20       (0x7)
		mov o4, c20       (0x7)
		; (dot(lightDir, v.normal)
		mul r0, c20,  v2  (0x8)
		mul r0, c20,  r0  (0x8)
		dp3 r0,  c8,  r0  (0x6)
		max r0, c20,  r0  (0x4)
		;   * sunDiffuse
		mul r0, c10,  r0  (0x6)
		;   + sunAmbient
		add r0,  c9,  r0  (0x6)
		; ) * v.rgba
		mul r0,  v3,  r0  (0x6)
		; scale down
		mul o1, c20,  r0  (0x8)
		mov o1, c20       (0x3)
		flush
		end
	endmain:
 
;operand descriptors
	.opdesc x___, xyzw, xyzw ; 0x0
	.opdesc _y__, xyzw, xyzw ; 0x1
	.opdesc __z_, xyzw, xyzw ; 0x2
	.opdesc ___w, xyzw, xyzw ; 0x3
	.opdesc xyz_, yyyy, xyzw ; 0x4
	.opdesc xyzw, xyzw, xyzw ; 0x5
	.opdesc xyz_, xyzw, xyzw ; 0x6
	.opdesc xyzw, yyyw, xyzw ; 0x7
	.opdesc xyz_, zzzz, xyzw ; 0x8
