(ReferenceLink (stv 1.0 1.0)
   (WordInstanceNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665")
   (WordNode "drop")
)
(WordInstanceLink (stv 1.0 1.0)
   (WordInstanceNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665")
   (ParseNode "sentence@e80cf258-1b7d-4db6-9907-0d569613e83d_parse_0")
)
(ReferenceLink (stv 1.0 1.0)
   (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a")
   (WordNode "it")
)
(WordInstanceLink (stv 1.0 1.0)
   (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a")
   (ParseNode "sentence@e80cf258-1b7d-4db6-9907-0d569613e83d_parse_0")
)
(ReferenceLink (stv 1.0 1.0)
   (ParseNode "sentence@e80cf258-1b7d-4db6-9907-0d569613e83d_parse_0")
   (ListLink
      (WordInstanceNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665")
      (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a")
   )
)
(EvaluationLink (stv 1.0 1.0)
   (LinkGrammarRelationshipNode "Ox")
   (ListLink
      (WordInstanceNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665")
      (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a")
   )
)
(EvaluationLink (stv 1.0 1.0)
   (LinkGrammarRelationshipNode "Wi")
   (ListLink
      (WordInstanceNode "LEFT-WALL@sentence@e80cf258-1b7d-4db6-9907-0d569613e83d_parse_0")
      (WordInstanceNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665")
   )
)
(ParseLink (stv 1 1)
   (ParseNode "sentence@e80cf258-1b7d-4db6-9907-0d569613e83d_parse_0" (stv 1.0 0.9762))
   (SentenceNode "sentence@e80cf258-1b7d-4db6-9907-0d569613e83d")
)
(LemmaLink (stv 1.0 1.0)
   (WordInstanceNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665")
   (WordNode "drop")
)
(LemmaLink (stv 1.0 1.0)
   (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a")
   (WordNode "it")
)
; gender (it, neuter)
(InheritanceLink (stv 1.0 1.0)
   (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a")
   (DefinedLinguisticConceptNode "neuter")
)
; pos (it, noun)
(PartOfSpeechLink (stv 1.0 1.0)
   (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a")
   (DefinedLinguisticConceptNode "noun")
)
; PRONOUN-FLAG (it, T)
(InheritanceLink (stv 1.0 1.0)
   (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a")
   (DefinedLinguisticConceptNode "pronoun")
)
; DEFINITE-FLAG (it, T)
(InheritanceLink (stv 1.0 1.0)
   (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a")
   (DefinedLinguisticConceptNode "definite")
)
; _obj (<<drop>>, <<it>>) 
(EvaluationLink (stv 1.0 1.0)
   (DefinedLinguisticRelationshipNode "_obj")
   (ListLink
      (WordInstanceNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665")
      (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a")
   )
)
; tense (drop, imperative)
(InheritanceLink (stv 1.0 1.0)
   (WordInstanceNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665")
   (DefinedLinguisticConceptNode "imperative")
)
; pos (drop, verb)
(PartOfSpeechLink (stv 1.0 1.0)
   (WordInstanceNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665")
   (DefinedLinguisticConceptNode "verb")
)
; inflection-TAG (drop, .v)
(InheritanceLink (stv 1.0 1.0)
   (WordInstanceNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665")
   (DefinedLinguisticConceptNode ".v")
)
; Transitive_action:Patient(drop@55205f02-ab42-4d0f-baaf-f7c225ad8665,it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a)
; Birth:Egg(drop@55205f02-ab42-4d0f-baaf-f7c225ad8665,it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a)
; Request:Message(drop@55205f02-ab42-4d0f-baaf-f7c225ad8665,drop@55205f02-ab42-4d0f-baaf-f7c225ad8665)
; Request:Addressee(drop@55205f02-ab42-4d0f-baaf-f7c225ad8665,you)
; Motion_directional:Direction(drop@55205f02-ab42-4d0f-baaf-f7c225ad8665,down)
; Motion_directional:Theme(drop@55205f02-ab42-4d0f-baaf-f7c225ad8665,it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a)
; Transitive_action:Agent(drop@55205f02-ab42-4d0f-baaf-f7c225ad8665,you)
; Motion_directional:Source(drop@55205f02-ab42-4d0f-baaf-f7c225ad8665,you)
; Motion_directional:Goal(drop@55205f02-ab42-4d0f-baaf-f7c225ad8665,floor)
; Motion_directional:Depictive(drop@55205f02-ab42-4d0f-baaf-f7c225ad8665,drop)
; New Frame Format Output

(InheritanceLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Transitive_action_Patient")
   (DefinedFrameElementNode "#Transitive_action:Patient")
)
(FrameElementLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Transitive_action")
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Transitive_action_Patient")
)
(EvaluationLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Transitive_action_Patient")
   (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a")
)
(InheritanceLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Transitive_action_Agent")
   (DefinedFrameElementNode "#Transitive_action:Agent")
)
(FrameElementLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Transitive_action")
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Transitive_action_Agent")
)
(EvaluationLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Transitive_action_Agent")
   (ConceptNode "#you")
)
(InheritanceLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Transitive_action")
   (DefinedFrameNode "#Transitive_action")
)
(InheritanceLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional_Direction")
   (DefinedFrameElementNode "#Motion_directional:Direction")
)
(FrameElementLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional")
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional_Direction")
)
(EvaluationLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional_Direction")
   (ConceptNode "#down")
)
(InheritanceLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional_Theme")
   (DefinedFrameElementNode "#Motion_directional:Theme")
)
(FrameElementLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional")
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional_Theme")
)
(EvaluationLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional_Theme")
   (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a")
)
(InheritanceLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional_Source")
   (DefinedFrameElementNode "#Motion_directional:Source")
)
(FrameElementLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional")
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional_Source")
)
(EvaluationLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional_Source")
   (ConceptNode "#you")
)
(InheritanceLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional_Goal")
   (DefinedFrameElementNode "#Motion_directional:Goal")
)
(FrameElementLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional")
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional_Goal")
)
(EvaluationLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional_Goal")
   (ConceptNode "#floor")
)
(InheritanceLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional_Depictive")
   (DefinedFrameElementNode "#Motion_directional:Depictive")
)
(FrameElementLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional")
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional_Depictive")
)
(EvaluationLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional_Depictive")
   (ConceptNode "#drop")
)
(InheritanceLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Motion_directional")
   (DefinedFrameNode "#Motion_directional")
)
(InheritanceLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Birth_Egg")
   (DefinedFrameElementNode "#Birth:Egg")
)
(FrameElementLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Birth")
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Birth_Egg")
)
(EvaluationLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Birth_Egg")
   (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a")
)
(InheritanceLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Birth")
   (DefinedFrameNode "#Birth")
)
(InheritanceLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Request_Message")
   (DefinedFrameElementNode "#Request:Message")
)
(FrameElementLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Request")
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Request_Message")
)
(EvaluationLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Request_Message")
   (WordInstanceNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665")
)
(InheritanceLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Request_Addressee")
   (DefinedFrameElementNode "#Request:Addressee")
)
(FrameElementLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Request")
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Request_Addressee")
)
(EvaluationLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Request_Addressee")
   (ConceptNode "#you")
)
(InheritanceLink (stv 1 1)
   (PredicateNode "drop@55205f02-ab42-4d0f-baaf-f7c225ad8665_Request")
   (DefinedFrameNode "#Request")
)
; END of New Frame Format Output

; ante(it, bone)
(EvaluationLink (stv 1 0.5)
   (ConceptNode "anaphoric reference")
   (ListLink 
      (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a" )
      (WordInstanceNode "bone@c7c2a7a5-1d0b-485e-b789-ed6cbf58b162" )
   )
)
; ante(it, bone)
(EvaluationLink (stv 1 0.3333333333333333)
   (ConceptNode "anaphoric reference")
   (ListLink 
      (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a" )
      (WordInstanceNode "bone@f059078c-6b11-4bfc-a837-f3b050f9830c" )
   )
)
; ante(it, bear)
(EvaluationLink (stv 1 0.25)
   (ConceptNode "anaphoric reference")
   (ListLink 
      (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a" )
      (WordInstanceNode "bear@dbf88055-d39d-4a5e-9077-672ee384330e" )
   )
)
; ante(it, stick)
(EvaluationLink (stv 1 0.2)
   (ConceptNode "anaphoric reference")
   (ListLink 
      (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a" )
      (WordInstanceNode "stick@13ba07b1-50b3-42fd-9c2f-374c9b979f10" )
   )
)

(EvaluationLink (stv 1 0.2)
   (ConceptNode "anaphoric reference")
   (ListLink 
      (WordInstanceNode "it@d605f2e6-98bd-47fa-b3e1-5b7b9dbc570a" )
      (WordInstanceNode "ball@2227209d-89f8-444f-ab90-d749bbc71eda")
   )
)


(ListLink (stv 1 1)   (AnchorNode "# New Parsed Sentence")   (SentenceNode "sentence@e80cf258-1b7d-4db6-9907-0d569613e83d")); END OF SENTENCE
