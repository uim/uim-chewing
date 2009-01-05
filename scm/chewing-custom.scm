;;; chewing-custom.scm: Customization variables for chewing.scm
;;;
;;; Copyright (c) 2006-2008 uim Project http://code.google.com/p/uim/
;;;
;;; All rights reserved.
;;;
;;; Redistribution and use in source and binary forms, with or without
;;; modification, are permitted provided that the following conditions
;;; are met:
;;; 1. Redistributions of source code must retain the above copyright
;;;    notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright
;;;    notice, this list of conditions and the following disclaimer in the
;;;    documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of authors nor the names of its contributors
;;;    may be used to endorse or promote products derived from this software
;;;    without specific prior written permission.
;;;
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
;;; ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;;; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
;;; ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
;;; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
;;; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
;;; OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
;;; HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
;;; LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
;;; OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
;;; SUCH DAMAGE.
;;;;

(require "i18n.scm")

(define chewing-im-name-label (N_ "Chewing"))
(define chewing-im-short-desc (N_ "Chewing Traditianl Chinese input method"))

(define chewing-cand-selection-numkey 0)
(define chewing-cand-selection-asdfghjkl 1)

(define-custom-group 'chewing
  (ugettext chewing-im-name-label)
  (ugettext chewing-im-short-desc))


(define-custom 'chewing-esc-clean? #f
  '(chewing)
  '(boolean)
  (_ "ESC cleans buffer")
  (_ "long description will be here."))

(define-custom 'chewing-phrase-forward? #f
  '(chewing)
  '(boolean)
  (_ "Phrase forward")
  (_ "long description will be here."))

(define-custom 'chewing-space-as-selection? #t
  '(chewing)
  '(boolean)
  (_ "Use space as selection")
  (_ "long description will be here."))

(define-custom 'chewing-phrase-choice-rearward? #f
  '(chewing)
  '(boolean)
  (_ "Phrase choice rearward")
  (_ "long description will be here."))

(define-custom 'chewing-auto-shift-cursor? #f
  '(chewing)
  '(boolean)
  (_ "Auto shift cursor")
  (_ "long description will be here."))

(define-custom 'chewing-candidate-selection-style 'chewing-cand-selection-numkey
  '(chewing candwin)
  (list 'choice
	(list 'chewing-cand-selection-numkey (_ "num key") (_ "Selection by number key"))
	(list 'chewing-cand-selection-asdfghjkl (_ "asdfghjkl;") (_ "Selection by asdfghjkl;")))
  (_ "Candidate selection style")
  (_ "long description will be here."))

(define-custom 'chewing-kbd-layout 'chewing-kbd-default
  '(chewing)
  (list 'choice
	(list 'chewing-kbd-default (_ "Default") (_ "Default Keyboard"))
	(list 'chewing-kbd-hsu (_ "HSU") (_ "HSU Keyboard"))
	(list 'chewing-kbd-ibm (_ "IBM") (_ "IBM Keyboard"))
	(list 'chewing-kbd-gin-yieh (_ "GIN YIEH") (_ "GIN YIEH Keyboard"))
	(list 'chewing-kbd-et (_ "ET") (_ "ET Keyboard"))
	(list 'chewing-kbd-et26 (_ "ET26") (_ "ET26 Keyboard"))
	(list 'chewing-kbd-dvorak (_ "DVORAK") (_ "DVORAK Keyboard"))
	(list 'chewing-kbd-dvorak-hsu (_ "DVORAK HSU") (_ "DVORAK HSU Keyboard"))
        (list 'chewing-kbd-dachen-cp26 (_ "DACHEN CP26") (_ "DACHEN CP26 Keyboard"))
	(list 'chewing-kbd-hanyu-pinyin (_ "HANYU PINYIN") (_ "HANYU PINYIN Keyboard")))
  (_ "Keyboard layout type")
  (_ "long description will be here."))


(custom-add-hook 'chewing-esc-clean?
		 'custom-set-hooks
		 (lambda ()
		   (if (and
			(symbol-bound? 'chewing-lib-init)
			chewing-lib-initialized?)
		       (chewing-lib-reload-config))))

(custom-add-hook 'chewing-phrase-forward?
		 'custom-set-hooks
		 (lambda ()
		   (if (and
			(symbol-bound? 'chewing-lib-init)
			chewing-lib-initialized?)
		       (chewing-lib-reload-config))))

(custom-add-hook 'chewing-space-as-selection?
		 'custom-set-hooks
		 (lambda ()
		   (if (and
			(symbol-bound? 'chewing-lib-init)
			chewing-lib-initialized?)
		       (chewing-lib-reload-config))))

(custom-add-hook 'chewing-candidate-selection-style
		 'custom-set-hooks
		 (lambda ()
		   (if (and
			(symbol-bound? 'chewing-lib-init)
			chewing-lib-initialized?)
		       (chewing-lib-reload-config))))

(custom-add-hook 'chewing-kbd-layout
		 'custom-set-hooks
		 (lambda ()
		   (if (and
			(symbol-bound? 'chewing-lib-init)
			chewing-lib-initialized?)
		       (chewing-lib-reload-config))))

(custom-add-hook 'chewing-phrase-choice-rearward?
		 'custom-set-hooks
		 (lambda ()
		   (if (and
			(symbol-bound? 'chewing-lib-init)
			chewing-lib-initialized?)
		       (chewing-lib-reload-config))))

(custom-add-hook 'chewing-auto-shift-cursor?
		 'custom-set-hooks
		 (lambda ()
		   (if (and
			(symbol-bound? 'chewing-lib-init)
			chewing-lib-initialized?)
		       (chewing-lib-reload-config))))
