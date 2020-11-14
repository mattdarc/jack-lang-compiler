; ModuleID = 'string.cpp'
source_filename = "string.cpp"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

%struct.String = type { i32, i32, i8* }

; Function Attrs: norecurse ssp uwtable
define i32 @main() local_unnamed_addr #0 {
  %1 = call %struct.String* @_ZN6String6createEi(i32 10)
  call void @_ZN6String3setEic(%struct.String* %1, i32 0, i8 signext 97)
  %2 = call signext i8 @_ZN6String3getEi(%struct.String* %1, i32 0)
  %3 = sext i8 %2 to i32
  ret i32 %3
}

; Function Attrs: ssp uwtable
define linkonce_odr %struct.String* @_ZN6String6createEi(i32 %0) local_unnamed_addr #1 align 2 {
  %2 = call dereferenceable(16) i8* @_Znwm(i64 16) #4
  %3 = bitcast i8* %2 to %struct.String*
  %4 = sext i32 %0 to i64
  %5 = call i8* @_Znam(i64 %4) #4
  %6 = getelementptr inbounds i8, i8* %2, i64 8
  %7 = bitcast i8* %6 to i8**
  store i8* %5, i8** %7, align 8, !tbaa !3
  %8 = bitcast i8* %2 to i32*
  store i32 %0, i32* %8, align 8, !tbaa !9
  %9 = getelementptr inbounds i8, i8* %2, i64 4
  %10 = bitcast i8* %9 to i32*
  store i32 0, i32* %10, align 4, !tbaa !10
  ret %struct.String* %3
}

; Function Attrs: nounwind ssp uwtable
define linkonce_odr void @_ZN6String3setEic(%struct.String* %0, i32 %1, i8 signext %2) local_unnamed_addr #2 align 2 {
  %4 = getelementptr inbounds %struct.String, %struct.String* %0, i64 0, i32 2
  %5 = load i8*, i8** %4, align 8, !tbaa !3
  %6 = sext i32 %1 to i64
  %7 = getelementptr inbounds i8, i8* %5, i64 %6
  store i8 %2, i8* %7, align 1, !tbaa !11
  ret void
}

; Function Attrs: nounwind ssp uwtable
define linkonce_odr signext i8 @_ZN6String3getEi(%struct.String* %0, i32 %1) local_unnamed_addr #2 align 2 {
  %3 = getelementptr inbounds %struct.String, %struct.String* %0, i64 0, i32 2
  %4 = load i8*, i8** %3, align 8, !tbaa !3
  %5 = sext i32 %1 to i64
  %6 = getelementptr inbounds i8, i8* %4, i64 %5
  %7 = load i8, i8* %6, align 1, !tbaa !11
  ret i8 %7
}

; Function Attrs: nobuiltin nofree
declare noalias nonnull i8* @_Znwm(i64) local_unnamed_addr #3

; Function Attrs: nobuiltin nofree
declare noalias nonnull i8* @_Znam(i64) local_unnamed_addr #3

attributes #0 = { norecurse ssp uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { ssp uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind ssp uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nobuiltin nofree "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { builtin }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{!"clang version 10.0.0 "}
!3 = !{!4, !8, i64 8}
!4 = !{!"_ZTS6String", !5, i64 0, !5, i64 4, !8, i64 8}
!5 = !{!"int", !6, i64 0}
!6 = !{!"omnipotent char", !7, i64 0}
!7 = !{!"Simple C++ TBAA"}
!8 = !{!"any pointer", !6, i64 0}
!9 = !{!4, !5, i64 0}
!10 = !{!4, !5, i64 4}
!11 = !{!6, !6, i64 0}
